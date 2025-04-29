import logging

import uvicorn
from fastapi import FastAPI, UploadFile
from fastapi.responses import JSONResponse
from PIL import Image
import os
from processing.car_detector import detect_car_plate
import shutil
app = FastAPI()

UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
log = logging.getLogger("uvicorn.error")

@app.post("/upload_picture/")
async def upload_picture(file: UploadFile):
    file_location = os.path.join(UPLOAD_FOLDER, file.filename)
    with open(file_location, "wb") as buffer:
        shutil.copyfileobj(file.file, buffer)

    return JSONResponse({"filename": file.filename, "saved_to": file_location})

@app.post("/detect_plate/")
async def detect_plate(file: UploadFile) -> JSONResponse:
    image = Image.open(file.file)
    car_plate = detect_car_plate(image)
    if not car_plate:
        return JSONResponse({"error": "No plate detected"}, status_code=400)

    return JSONResponse({"plate": car_plate})


def main() -> None:
    uvicorn.run(app,  host="0.0.0.0", port=8000)


def dev() -> None:
    uvicorn.run("server.server:app", host="0.0.0.0", port=8000, reload=True)


if __name__ == "__main__":
    main()
