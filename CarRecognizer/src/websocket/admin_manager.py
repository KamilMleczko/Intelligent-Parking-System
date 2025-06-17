import logging

from fastapi import WebSocket

from .base_manager import WSManagerInterface

log = logging.getLogger("uvicorn.error")


class AdminConnectionManager(WSManagerInterface):
    def __init__(self):
        self.active_connections: list[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)
        log.info(f"Admin connected. Count: {len(self.active_connections)}")

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
        log.info(f"Admin disconnected. Count: {len(self.active_connections)}")


# expose singleton
admin_manager = AdminConnectionManager()
