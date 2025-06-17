import logging

from fastapi import APIRouter, Depends, Form, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
from sqlmodel import select

from db.models import EntryLog, Plate
from db.session import get_session
from websocket.admin_manager import admin_manager

router = APIRouter()
log = logging.getLogger("uvicorn.error")
templates = Jinja2Templates(directory="src/templates")


@router.websocket("/ws/admin")
async def ws_admin(ws: WebSocket):
    await admin_manager.connect(ws)
    try:
        while True:
            await ws.receive_text()
    except WebSocketDisconnect:
        log.info("Admin disconnected")
        admin_manager.disconnect(ws)


@router.get("/admin", response_class=HTMLResponse)
async def admin_page(request: Request, session=Depends(get_session)):
    plates = session.exec(select(Plate).order_by(Plate.when.desc())).all()
    logs = session.exec(
        select(EntryLog).order_by(EntryLog.timestamp.desc()),
    ).all()
    return templates.TemplateResponse(
        "admin.html",
        {"request": request, "plates": plates, "logs": logs},
    )


@router.post("/admin/register")
async def register_plate(
    request: Request,
    plate_text: str = Form(...),
    session=Depends(get_session),
):
    new = Plate(text=plate_text)
    session.add(new)
    session.commit()
    return RedirectResponse(url="/admin", status_code=303)


@router.post("/admin/remove")
async def remove_plate(
    request: Request,
    plate_id: int = Form(...),
    session=Depends(get_session),
):
    plate = session.get(Plate, plate_id)
    if plate:
        session.delete(plate)
        session.commit()
    return RedirectResponse(url="/admin", status_code=303)
