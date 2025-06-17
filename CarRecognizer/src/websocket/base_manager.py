from abc import ABC, abstractmethod

from fastapi import WebSocket


class WSManagerInterface(ABC):
    @abstractmethod
    async def connect(self, websocket: WebSocket, *args, **kwargs):
        """Accept and register a websocket connection"""

    @abstractmethod
    def disconnect(self, *args, **kwargs):
        """Remove a websocket connection"""
