from fastapi import APIRouter, HTTPException, UploadFile
from PIL import Image

from processing.car_detector import detect_car_plate
from processing.car_model_recognizer import predict_car_model

router = APIRouter()


@router.post("/detect_plate/")
async def detect_plate(file: UploadFile):
    image = Image.open(file.file)
    plate = detect_car_plate(image)
    if not plate:
        raise HTTPException(status_code=400, detail="No plate detected")
    return {"plate": plate}


@router.post("/detect_model/")
async def detect_model(file: UploadFile):
    image = Image.open(file.file)
    result = predict_car_model(image)
    if not result:
        raise HTTPException(status_code=400, detail="No model detected")
    name, conf = result
    return {"model": name, "confidence": conf}
