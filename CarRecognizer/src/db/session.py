from sqlmodel import Session as SQLSession
from sqlmodel import SQLModel, create_engine

sqlite_file = "plates.db"
engine = create_engine(f"sqlite:///{sqlite_file}", echo=False)
SQLModel.metadata.create_all(engine)


def get_session():
    with SQLSession(engine) as session:
        yield session
