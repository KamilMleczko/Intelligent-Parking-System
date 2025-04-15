from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

import easyocr
import numpy as np
from PIL import Image
from ultralytics import YOLO

if TYPE_CHECKING:
    from ultralytics.engine.results import Boxes


@dataclass
class CarPlatePrediction:
    plate_number: str
    confidence: float


MODEL_PATH = Path("models/carPlateDetector.pt")

reader = easyocr.Reader(["en"])
car_plate_recognizer = YOLO(MODEL_PATH)


def read_text_from_bbox(
    image: Image.Image,
    bbox: tuple[int, int, int, int],
) -> str | None:
    """
    Reads text from a bounding box in an image using OCR.

    Parameters:
        image: The image to read text from.
        bbox: The bounding box coordinates (x1, y1, x2, y2).

    Returns:
        The text extracted from the image within the bounding box.
    """
    cropped_image = image.crop(bbox)

    ocr_result = reader.readtext(np.array(cropped_image))

    # we assume that in a bounding box of a detected car plate, there is only one text
    # so we take the first result
    if ocr_result:
        return ocr_result[0][1]

    return None


def detect_car_plate(image: Image.Image) -> str | None:
    results = car_plate_recognizer.predict(image, conf=0.5, show=False)
    # We assume that there is only one car on the picture
    first = results[0]
    boxes: Boxes | None = first.boxes
    predictions: list[CarPlatePrediction] = []
    if not boxes:
        return None
    for box in boxes:
        for xyxy in box.xyxy:
            predicted_text = read_text_from_bbox(image, tuple(map(int, xyxy)))
            if not predicted_text:
                continue
            predictions.append(CarPlatePrediction(predicted_text, float(box.conf[0])))

    highest_confidence_prediction = max(predictions, key=lambda x: x.confidence)
    return highest_confidence_prediction.plate_number
