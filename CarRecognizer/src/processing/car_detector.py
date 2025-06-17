import logging
import time
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

import cv2
import easyocr
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image
from ultralytics import YOLO

if TYPE_CHECKING:
    from ultralytics.engine.results import Results


@dataclass
class CarPlatePrediction:
    plate_number: str
    confidence: float


log = logging.getLogger("uvicorn.error")

MODEL_PATH = Path("models/carPlateDetector.pt")

reader = easyocr.Reader(
    ["en"],
    gpu=True,
    detect_network="craft",
    recog_network="standard",
    verbose=False,
)
car_plate_recognizer = YOLO(MODEL_PATH)


def estimate_skew_angle(
    gray: np.ndarray,
    delta: float = 0.5,
    limit: float = 15.0,
) -> float:
    h, w = gray.shape
    center = (w // 2, h // 2)

    best_score = -1
    best_angle = 0.0
    angles = np.arange(-limit, limit + delta, delta)

    for angle in angles:
        M = cv2.getRotationMatrix2D(center, angle, 1.0)
        rot = cv2.warpAffine(
            gray,
            M,
            (w, h),
            flags=cv2.INTER_CUBIC,
            borderMode=cv2.BORDER_REPLICATE,
        )
        proj = np.sum(rot, axis=1)  # horizontal projection
        score = float(np.std(proj))  # more “peaky” when text is level
        if score > best_score:
            best_score = score
            best_angle = angle

    return best_angle


def deskew_by_projection(
    pil_img: Image.Image,
    delta: float = 0.5,
    limit: float = 15.0,
) -> Image.Image:
    img = np.array(pil_img)
    gray = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)

    angle = estimate_skew_angle(gray, delta=delta, limit=limit)

    h, w = gray.shape
    center = (w // 2, h // 2)
    M = cv2.getRotationMatrix2D(center, angle, 1.0)
    rotated = cv2.warpAffine(
        img,
        M,
        (w, h),
        flags=cv2.INTER_CUBIC,
        borderMode=cv2.BORDER_REPLICATE,
    )

    gray_rot = cv2.cvtColor(rotated, cv2.COLOR_RGB2GRAY)
    _, thresh = cv2.threshold(gray_rot, 1, 255, cv2.THRESH_BINARY)
    ys, xs = np.where(thresh > 0)
    if len(xs) and len(ys):
        x0, x1 = xs.min(), xs.max()
        y0, y1 = ys.min(), ys.max()
        cropped = rotated[y0 : y1 + 1, x0 : x1 + 1]
    else:
        cropped = rotated  # fallback if threshold fails

    return Image.fromarray(cropped)


def preprocess_plate(pil_img: Image.Image, debug: bool = False) -> Image.Image:
    deskewed = deskew_by_projection(pil_img, delta=0.5, limit=15)

    arr = np.array(deskewed)
    gray = cv2.cvtColor(arr, cv2.COLOR_RGB2GRAY)

    clahe = cv2.createCLAHE(clipLimit=3.0, tileGridSize=(8, 8))
    img_clahe = clahe.apply(gray)

    blur = cv2.GaussianBlur(img_clahe, (0, 0), sigmaX=3)
    img_sharp = cv2.addWeighted(img_clahe, 1.5, blur, -0.5, 0)

    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    img_closed = cv2.morphologyEx(img_sharp, cv2.MORPH_CLOSE, kernel)
    if debug:
        fig, axs = plt.subplots(nrows=1, ncols=3, figsize=(8, 4))
        axs[0].imshow(pil_img)
        axs[0].set_title("Original Plate")
        axs[1].imshow(deskewed)
        axs[1].set_title("Deskewed Plate")
        axs[2].imshow(img_closed, cmap="gray")
        axs[2].set_title("Processed Plate")
        plt.tight_layout()
        plt.show()

    return Image.fromarray(img_closed)


def ocr_merge_boxes(img: Image.Image) -> tuple[str, float] | None:
    """
    1) Run EasyOCR on the patch
    2) Gather all box heights and compute max_h
    3) Prune any box with height < 0.7 * max_h
    4) Merge the rest left→right, average confidence
    """
    arr = np.array(img)
    try:
        results = reader.readtext(
            arr,
            decoder="beamsearch",
            paragraph=False,
            allowlist="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
            detail=1,
            beamWidth=20,
            width_ths=0.7,
            text_threshold=0.2,
            contrast_ths=0.15,
            adjust_contrast=1.25,
            link_threshold=0.5,
            mag_ratio=2.0,
        )
        if not results:
            return None

        # compute each box's height
        heights = []
        for box, text, conf in results:
            ys = [pt[1] for pt in box]
            heights.append(max(ys) - min(ys))
        max_h = max(heights)

        # prune those < 70% of max_h
        threshold = 0.7 * max_h
        filtered = []
        for (box, text, conf), h in zip(results, heights, strict=False):
            if h >= threshold:
                filtered.append((box, text, conf))

        if not filtered:
            log.info("All boxes pruned—no plate text remains")
            return None

        # sort left→right and merge
        filtered.sort(key=lambda r: min(pt[0] for pt in r[0]))
        texts = [r[1] for r in filtered]
        confs = [r[2] for r in filtered]

        merged = "".join(texts)
        avg_conf = float(sum(confs) / len(confs))
        return merged, avg_conf

    except Exception:
        log.exception("OCR prune‐merge failed")
        return None


def detect_car_plate(image: Image.Image) -> str | None:
    start = time.perf_counter()
    try:
        results: Results = car_plate_recognizer.predict(
            image,
            conf=0.5,
            show=False,
        )[0]
        boxes = results.boxes
        if not boxes:
            log.info("No plates detected by YOLO")
            return None

        best_plate = None
        best_score = 0.0

        for box in boxes:
            x1, y1, x2, y2 = map(int, box.xyxy[0])
            patch = image.crop((x1, y1, x2, y2))

            patch = preprocess_plate(patch)
            o = ocr_merge_boxes(patch)
            if o:
                text, conf = o
                # optionally weigh by YOLO box.confidence
                score = conf * float(box.conf[0])
                if score > best_score:
                    best_plate, best_score = text, score

        if not best_plate:
            log.info("OCR found no valid text")
            return None
        best_plate = "".join(best_plate.split())
        log.info(f"Plate → `{best_plate}` (score {best_score:.2f})")
        return best_plate

    except Exception:
        log.exception("detect_car_plate failed")
        return None

    finally:
        elapsed = (time.perf_counter() - start) * 1000
        log.info(f"Plate detection pipeline took {elapsed:.1f} ms")
