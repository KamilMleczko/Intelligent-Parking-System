import json
import logging

from fastapi import WebSocket

from .base_manager import WSManagerInterface

log = logging.getLogger("uvicorn.error")


class GateConnectionManager(WSManagerInterface):
    def __init__(self):
        self.active_connections: list[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
        log.info(f"Gate device connected: {len(self.active_connections)} total")

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
        log.info(f"Gate device disconnected: {len(self.active_connections)} total")

    async def send(self, command: dict):
        """Send open/close command: {'action': 'open'|'close'}"""
        data = json.dumps(command)
        for ws in list(self.active_connections):
            try:
                await ws.send_text(data)
            except Exception:
                self.disconnect(ws)


gate_manager = GateConnectionManager()
