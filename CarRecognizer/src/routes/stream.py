import logging

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from websocket.stream_manager import stream_manager

router = APIRouter()
log = logging.getLogger("uvicorn.error")


@router.websocket("/ws_stream/{client_id}")
async def ws_stream(websocket: WebSocket, client_id: str):
    await stream_manager.connect(websocket, client_id)
    try:
        while True:
            data = await websocket.receive_bytes()
            if not data:
                log.warning(f"Empty frame from {client_id}, closing connection.")
                break
            detected = bool(data[0])
            frame = data[1:]
            log.info(f"Received frame from {client_id}: size={len(frame)} bytes")
            log.info(f"{detected=} frame={frame[:16]}...")
            await stream_manager.receive_frame(client_id, frame, detected)
    except WebSocketDisconnect:
        log.info(f"Stream disconnected: {client_id}")
        stream_manager.disconnect(client_id)
    except Exception:
        log.exception(f"Stream error for {client_id}")
        stream_manager.disconnect(client_id)


@router.get("/stream_status")
async def stream_status():
    return {
        "active_clients": list(stream_manager.active_connections.keys()),
        "total_clients": len(stream_manager.active_connections),
    }
