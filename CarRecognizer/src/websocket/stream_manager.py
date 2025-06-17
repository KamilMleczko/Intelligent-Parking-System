import io
import logging
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Literal

from fastapi import WebSocket
from PIL import Image
from sqlmodel import Session, select

from db.models import Plate
from db.session import engine
from processing.car_detector import detect_car_plate
from processing.car_model_recognizer import predict_car_model
from services.plate_service import log_entry

from .base_manager import WSManagerInterface
from .gate_manager import gate_manager

log = logging.getLogger("uvicorn.error")


@dataclass
class GateCommand:
    action: Literal["open", "close"]
    car_plate: str


class StreamConnectionManager(WSManagerInterface):
    def __init__(self):
        self.active_connections: dict[str, WebSocket] = {}
        self.latest_frames: dict[str, bytes] = {}

    async def connect(self, websocket: WebSocket, client_id: str):
        await websocket.accept()
        self.active_connections[client_id] = websocket
        self.latest_frames[client_id] = b""
        log.info(f"Client {client_id} connected. Total: {len(self.active_connections)}")

    def disconnect(self, client_id: str):
        self.active_connections.pop(client_id, None)
        self.latest_frames.pop(client_id, None)
        log.info(
            f"Client {client_id} disconnected. Total: {len(self.active_connections)}",
        )

    async def receive_frame(self, client_id: str, data: bytes, object_detected: bool):
        if client_id not in self.active_connections:
            return
        self.latest_frames[client_id] = data

        if object_detected:
            try:
                img = Image.open(io.BytesIO(data))
                plate_text = detect_car_plate(img)
                model_res = predict_car_model(img)
                model_name = model_res[0] if model_res else None

                # check authorization
                with Session(engine) as session:
                    record = session.exec(
                        select(Plate).where(Plate.text == plate_text),
                    ).first()
                    authorized = record is not None
                if plate_text:
                    await log_entry(plate_text, model_name)

                if plate_text and authorized:
                    log.info(f"Authorized entry: {plate_text}")
                else:
                    log.info(f"Denied entry: {plate_text}")

                command = GateCommand(
                    action="open" if authorized else "close",
                    car_plate=plate_text or "",
                )
                # if plate_text:
                if True:
                    timestamp = int(time.time())
                    img_path = Path(f"images/{timestamp}_{plate_text}.jpg")
                    img_path.parent.mkdir(parents=True, exist_ok=True)
                    img.save(img_path)
                    log.info(f"Saved image to {img_path}")
                await gate_manager.send(asdict(command))
            except Exception as e:
                log.error(f"Processing error: {e}")


# singleton instance
stream_manager = StreamConnectionManager()
