import logging

import uvicorn
from fastapi import FastAPI, UploadFile
from fastapi.responses import JSONResponse
from PIL import Image

from processing.car_detector import detect_car_plate

app = FastAPI()

log = logging.getLogger("uvicorn.error")


@app.post("/detect_plate/")
async def detect_plate(file: UploadFile) -> JSONResponse:
    image = Image.open(file.file)
    car_plate = detect_car_plate(image)
    if not car_plate:
        return JSONResponse({"error": "No plate detected"}, status_code=400)

    return JSONResponse({"plate": car_plate})


def main() -> None:
    uvicorn.run(app, host="localhost", port=8000)


def dev() -> None:
    uvicorn.run("server.server:app", host="localhost", port=8000, reload=True)


if __name__ == "__main__":
    main()
