from fastapi import FastAPI, UploadFile, WebSocket, WebSocketDisconnect
import uvicorn
import logging
import os
import shutil
from PIL import Image
from processing.car_detector import detect_car_plate
from fastapi.responses import JSONResponse
import asyncio
import io
import base64
import time


app = FastAPI()

UPLOAD_FOLDER = "uploads"
STREAM_FOLDER = "stream"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(STREAM_FOLDER, exist_ok=True)
log = logging.getLogger("uvicorn.error")

class ConnectionManager:
    def __init__(self):
        self.active_connections = {}
        self.latest_frames = {}

    async def connect(self, websocket: WebSocket, client_id: str):
        await websocket.accept()
        self.active_connections[client_id] = websocket
        self.latest_frames[client_id] = None
        log.info(f"Client {client_id} connected. Total clients: {len(self.active_connections)}")

    def disconnect(self, client_id: str):
        if client_id in self.active_connections:
            del self.active_connections[client_id]
            del self.latest_frames[client_id]
            log.info(f"Client {client_id} disconnected. Total clients: {len(self.active_connections)}")

    async def receive_frame(self, client_id: str, data: bytes, object_detected: bool):
        """Process incoming frame data from the ESP32 camera"""
        if client_id in self.active_connections:
            # Save the latest frame for this client
            self.latest_frames[client_id] = data
            
            # Optional: save the frame to disk with timestamp
            timestamp = int(time.time())
            filename = f"{STREAM_FOLDER}/{client_id}_{timestamp}.jpg"
            
            log.debug(filename)
            with open(filename, "wb") as f:
                f.write(data)
                
            # Process the frame for car plate detection only if object is detected
            if object_detected:
                try:
                    image = Image.open(io.BytesIO(data))
                    car_plate = detect_car_plate(image)
                    if car_plate:
                        log.info(f"Detected license plate: {car_plate} from client {client_id}")
                        # You could store this in a database or send a notification
                except Exception as e:
                    log.error(f"Error processing frame: {str(e)}")
            else:
                log.info(f"No objects in front of camera for client {client_id}")

manager = ConnectionManager()

@app.websocket("/ws_stream/{client_id}")
async def websocket_endpoint(websocket: WebSocket, client_id: str):
    await manager.connect(websocket, client_id)
    try:
        # Log client connection
        log.info(f"New WebSocket connection from client {client_id}")
        
        while True:
            # ESP32 will send two separate websocket frames:
            # 1. First a small frame with object detection status
            # 2. Then the image data frame
            
            # Receive the binary message with object detection status
            status_data = await websocket.receive_bytes()
            # The ESP32 sends a single byte where 1 = object detected, 0 = no object
            object_detected = bool(status_data[0]) if status_data else False
            log.debug(f"Received object detection status from {client_id}: {object_detected}")
            
            # Now receive the image frame data
            frame_data = await websocket.receive_bytes()
            log.debug(f"Received frame from {client_id}, size: {len(frame_data)} bytes")
            
            # Process both the status and frame
            await manager.receive_frame(client_id, frame_data, object_detected)
            
            # Send acknowledgment back to the client
            await websocket.send_text("Frame received")
    except WebSocketDisconnect:
        log.info(f"WebSocket disconnected for client {client_id}")
        manager.disconnect(client_id)
    except Exception as e:
        log.error(f"WebSocket error for client {client_id}: {str(e)}")
        manager.disconnect(client_id)

@app.post("/upload_picture/")
async def upload_picture(file: UploadFile):
    file_location = os.path.join(UPLOAD_FOLDER, file.filename)
    with open(file_location, "wb") as buffer:
        shutil.copyfileobj(file.file, buffer)

    return JSONResponse({"filename": file.filename, "saved_to": file_location})

@app.post("/detect_plate/")
async def detect_plate(file: UploadFile) -> JSONResponse:
    image = Image.open(file.file)
    car_plate = detect_car_plate(image)
    if not car_plate:
        return JSONResponse({"error": "No plate detected"}, status_code=400)

    return JSONResponse({"plate": car_plate})

@app.get("/stream_status")
async def stream_status():
    """Return status of all connected streaming clients"""
    return {
        "active_clients": list(manager.active_connections.keys()),
        "total_clients": len(manager.active_connections)
    }


def main() -> None:
     uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        ws="websockets",       
        ws_ping_interval=15,           
        ws_ping_timeout=15,          
        timeout_keep_alive=30         
    )


def dev() -> None:
    uvicorn.run(
        "server.server:app",
        host="0.0.0.0", 
        port=8000, 
        reload=True,
        ws="websockets",               
        ws_ping_interval=15,           
        ws_ping_timeout=15,            
        timeout_keep_alive=30          
        )


if __name__ == "__main__":
    main()
