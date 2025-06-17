import logging
from pathlib import Path

import uvicorn
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from starlette.middleware.sessions import SessionMiddleware

from routes.admin_panel import router as admin_router
from routes.auth import router as auth_router
from routes.detect import router as detect_router
from routes.gate import router as gate_router
from routes.stream import router as stream_router

app = FastAPI()

# static & templates
app.mount("/static", StaticFiles(directory="src/static"), name="static")
templates = Jinja2Templates(directory="src/templates")

# sessions (demo credentials)
app.add_middleware(SessionMiddleware, secret_key="demo-secret-key")

# mount routers
app.include_router(detect_router)
app.include_router(stream_router)
app.include_router(admin_router)
app.include_router(auth_router)
app.include_router(gate_router)

log = logging.getLogger("uvicorn.error")
STREAM_FOLDER = Path("stream")
STREAM_FOLDER.mkdir(parents=True, exist_ok=True)


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
        app,
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
