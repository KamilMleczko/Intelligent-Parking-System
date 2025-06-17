from datetime import datetime

from sqlmodel import Field, SQLModel


class Plate(SQLModel, table=True):
    id: int = Field(default=None, primary_key=True)
    text: str
    when: datetime = Field(default_factory=datetime.utcnow)


class EntryLog(SQLModel, table=True):
    id: int = Field(default=None, primary_key=True)
    plate_text: str
    model_name: str | None = None
    timestamp: datetime = Field(default_factory=datetime.utcnow)
