from sqlmodel import Session

from db.models import EntryLog, Plate
from db.session import engine


async def create_plate(session: Session, text: str) -> Plate:
    plate = Plate(text=text)
    session.add(plate)
    session.commit()
    session.refresh(plate)
    return plate


async def log_entry(plate_text: str, model_name: str | None) -> EntryLog:
    """
    Persist each scan into EntryLog.
    """
    # we open a fresh session here because manager is outside HTTP context
    with Session(engine) as session:
        log = EntryLog(plate_text=plate_text, model_name=model_name)
        session.add(log)
        session.commit()
        session.refresh(log)
    return log
