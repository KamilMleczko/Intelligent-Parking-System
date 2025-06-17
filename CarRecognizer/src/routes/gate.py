# src/fastapi_admin/routes/gate.py
import logging

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from websocket.gate_manager import gate_manager

router = APIRouter()
log = logging.getLogger("uvicorn.error")


@router.websocket("/ws/gate")
async def ws_gate(websocket: WebSocket):
    """
    Gate devices connect here, send nothing, and will receive
    {"action":"open","car_plate":...} or {"action":"close"} messages.
    """
    await gate_manager.connect(websocket)
    try:
        # keep connection open until client disconnects
        while True:
            # we ignore any incoming message, but must await to detect disconnects
            await websocket.receive_text()
    except WebSocketDisconnect:
        log.info("Gate device disconnected")
        gate_manager.disconnect(websocket)
