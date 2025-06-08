import io
import logging
import os
import shutil
import time

import uvicorn
from fastapi import FastAPI, UploadFile, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse
from PIL import Image

from processing.car_detector import detect_car_plate, predict_car_model

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
        log.info(
            f"Client {client_id} connected. Total clients: {len(self.active_connections)}",
        )

    def disconnect(self, client_id: str):
        if client_id in self.active_connections:
            del self.active_connections[client_id]
            del self.latest_frames[client_id]
            log.info(
                f"Client {client_id} disconnected. Total clients: {len(self.active_connections)}",
            )

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
                    car_model_result = predict_car_model(image)
                    if car_plate:
                        log.info(f"Detected license plate: {car_plate} from client {client_id}")
                        # You could store this in a database or send a notification

                    if car_model_result:
                        model_name, confidence = car_model_result
                        log.info(
                            f"Detected car model: {model_name} (confidence: {confidence:.2f}) from client {client_id}",
                        )
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
        log.error(f"WebSocket error for client {client_id}: {e!s}")
        manager.disconnect(client_id)



@app.post("/detect_plate/")
async def detect_plate(file: UploadFile) -> JSONResponse:
    image = Image.open(file.file)
    car_plate = detect_car_plate(image)
    if not car_plate:
        return JSONResponse({"error": "No plate detected"}, status_code=400)

    return JSONResponse({"plate": car_plate})


@app.post("/detect_model/")
async def detect_model(file: UploadFile) -> JSONResponse:
    image = Image.open(file.file)
    car_model_result = predict_car_model(image)
    if not car_model_result:
        return JSONResponse({"error": "No model detected"}, status_code=400)

    model_name, confidence = car_model_result
    return JSONResponse(
        {
            "model": model_name,
            "confidence": confidence,
        },
    )


@app.get("/stream_status")
async def stream_status():
    """Return status of all connected streaming clients"""
    return {
        "active_clients": list(manager.active_connections.keys()),
        "total_clients": len(manager.active_connections),
    }


def main() -> None:
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=8000,
        ws="websockets",
        ws_ping_interval=15,
        ws_ping_timeout=15,
        timeout_keep_alive=30,
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
        timeout_keep_alive=30,
    )


if __name__ == "__main__":
    main()
