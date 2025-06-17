from dataclasses import dataclass
from pathlib import Path

import torch
import torchvision.transforms as T
from PIL import Image
from timm import create_model


@dataclass
class CarModelPrediction:
    model_name: str
    confidence: float


CarModel_MODEL_PATH = Path("models/carModelRecognizer.pth")

VOCABULARY = [
    "AM General Hummer SUV 2000",
    "Acura Integra Type R 2001",
    "Acura RL Sedan 2012",
    "Acura TL Sedan 2012",
    "Acura TL Type-S 2008",
    "Acura TSX Sedan 2012",
    "Acura ZDX Hatchback 2012",
    "Aston Martin V8 Vantage Convertible 2012",
    "Aston Martin V8 Vantage Coupe 2012",
    "Aston Martin Virage Convertible 2012",
    "Aston Martin Virage Coupe 2012",
    "Audi 100 Sedan 1994",
    "Audi 100 Wagon 1994",
    "Audi A5 Coupe 2012",
    "Audi R8 Coupe 2012",
    "Audi RS 4 Convertible 2008",
    "Audi S4 Sedan 2007",
    "Audi S4 Sedan 2012",
    "Audi S5 Convertible 2012",
    "Audi S5 Coupe 2012",
    "Audi S6 Sedan 2011",
    "Audi TT Hatchback 2011",
    "Audi TT RS Coupe 2012",
    "Audi TTS Coupe 2012",
    "Audi V8 Sedan 1994",
    "BMW 1 Series Convertible 2012",
    "BMW 1 Series Coupe 2012",
    "BMW 3 Series Sedan 2012",
    "BMW 3 Series Wagon 2012",
    "BMW 6 Series Convertible 2007",
    "BMW ActiveHybrid 5 Sedan 2012",
    "BMW M3 Coupe 2012",
    "BMW M5 Sedan 2010",
    "BMW M6 Convertible 2010",
    "BMW X3 SUV 2012",
    "BMW X5 SUV 2007",
    "BMW X6 SUV 2012",
    "BMW Z4 Convertible 2012",
    "Bentley Arnage Sedan 2009",
    "Bentley Continental Flying Spur Sedan 2007",
    "Bentley Continental GT Coupe 2007",
    "Bentley Continental GT Coupe 2012",
    "Bentley Continental Supersports Conv. Convertible 2012",
    "Bentley Mulsanne Sedan 2011",
    "Bugatti Veyron 16.4 Convertible 2009",
    "Bugatti Veyron 16.4 Coupe 2009",
    "Buick Enclave SUV 2012",
    "Buick Rainier SUV 2007",
    "Buick Regal GS 2012",
    "Buick Verano Sedan 2012",
    "Cadillac CTS-V Sedan 2012",
    "Cadillac Escalade EXT Crew Cab 2007",
    "Cadillac SRX SUV 2012",
    "Chevrolet Avalanche Crew Cab 2012",
    "Chevrolet Camaro Convertible 2012",
    "Chevrolet Cobalt SS 2010",
    "Chevrolet Corvette Convertible 2012",
    "Chevrolet Corvette Ron Fellows Edition Z06 2007",
    "Chevrolet Corvette ZR1 2012",
    "Chevrolet Express Cargo Van 2007",
    "Chevrolet Express Van 2007",
    "Chevrolet HHR SS 2010",
    "Chevrolet Impala Sedan 2007",
    "Chevrolet Malibu Hybrid Sedan 2010",
    "Chevrolet Malibu Sedan 2007",
    "Chevrolet Monte Carlo Coupe 2007",
    "Chevrolet Silverado 1500 Classic Extended Cab 2007",
    "Chevrolet Silverado 1500 Extended Cab 2012",
    "Chevrolet Silverado 1500 Hybrid Crew Cab 2012",
    "Chevrolet Silverado 1500 Regular Cab 2012",
    "Chevrolet Silverado 2500HD Regular Cab 2012",
    "Chevrolet Sonic Sedan 2012",
    "Chevrolet Tahoe Hybrid SUV 2012",
    "Chevrolet TrailBlazer SS 2009",
    "Chevrolet Traverse SUV 2012",
    "Chrysler 300 SRT-8 2010",
    "Chrysler Aspen SUV 2009",
    "Chrysler Crossfire Convertible 2008",
    "Chrysler PT Cruiser Convertible 2008",
    "Chrysler Sebring Convertible 2010",
    "Chrysler Town and Country Minivan 2012",
    "Daewoo Nubira Wagon 2002",
    "Dodge Caliber Wagon 2007",
    "Dodge Caliber Wagon 2012",
    "Dodge Caravan Minivan 1997",
    "Dodge Challenger SRT8 2011",
    "Dodge Charger SRT-8 2009",
    "Dodge Charger Sedan 2012",
    "Dodge Dakota Club Cab 2007",
    "Dodge Dakota Crew Cab 2010",
    "Dodge Durango SUV 2007",
    "Dodge Durango SUV 2012",
    "Dodge Journey SUV 2012",
    "Dodge Magnum Wagon 2008",
    "Dodge Ram Pickup 3500 Crew Cab 2010",
    "Dodge Ram Pickup 3500 Quad Cab 2009",
    "Dodge Sprinter Cargo Van 2009",
    "Eagle Talon Hatchback 1998",
    "FIAT 500 Abarth 2012",
    "FIAT 500 Convertible 2012",
    "Ferrari 458 Italia Convertible 2012",
    "Ferrari 458 Italia Coupe 2012",
    "Ferrari California Convertible 2012",
    "Ferrari FF Coupe 2012",
    "Fisker Karma Sedan 2012",
    "Ford E-Series Wagon Van 2012",
    "Ford Edge SUV 2012",
    "Ford Expedition EL SUV 2009",
    "Ford F-150 Regular Cab 2007",
    "Ford F-150 Regular Cab 2012",
    "Ford F-450 Super Duty Crew Cab 2012",
    "Ford Fiesta Sedan 2012",
    "Ford Focus Sedan 2007",
    "Ford Freestar Minivan 2007",
    "Ford GT Coupe 2006",
    "Ford Mustang Convertible 2007",
    "Ford Ranger SuperCab 2011",
    "GMC Acadia SUV 2012",
    "GMC Canyon Extended Cab 2012",
    "GMC Savana Van 2012",
    "GMC Terrain SUV 2012",
    "GMC Yukon Hybrid SUV 2012",
    "Geo Metro Convertible 1993",
    "HUMMER H2 SUT Crew Cab 2009",
    "HUMMER H3T Crew Cab 2010",
    "Honda Accord Coupe 2012",
    "Honda Accord Sedan 2012",
    "Honda Odyssey Minivan 2007",
    "Honda Odyssey Minivan 2012",
    "Hyundai Accent Sedan 2012",
    "Hyundai Azera Sedan 2012",
    "Hyundai Elantra Sedan 2007",
    "Hyundai Elantra Touring Hatchback 2012",
    "Hyundai Genesis Sedan 2012",
    "Hyundai Santa Fe SUV 2012",
    "Hyundai Sonata Hybrid Sedan 2012",
    "Hyundai Sonata Sedan 2012",
    "Hyundai Tucson SUV 2012",
    "Hyundai Veloster Hatchback 2012",
    "Hyundai Veracruz SUV 2012",
    "Infiniti G Coupe IPL 2012",
    "Infiniti QX56 SUV 2011",
    "Isuzu Ascender SUV 2008",
    "Jaguar XK XKR 2012",
    "Jeep Compass SUV 2012",
    "Jeep Grand Cherokee SUV 2012",
    "Jeep Liberty SUV 2012",
    "Jeep Patriot SUV 2012",
    "Jeep Wrangler SUV 2012",
    "Lamborghini Aventador Coupe 2012",
    "Lamborghini Diablo Coupe 2001",
    "Lamborghini Gallardo LP 570-4 Superleggera 2012",
    "Lamborghini Reventon Coupe 2008",
    "Land Rover LR2 SUV 2012",
    "Land Rover Range Rover SUV 2012",
    "Lincoln Town Car Sedan 2011",
    "MINI Cooper Roadster Convertible 2012",
    "Maybach Landaulet Convertible 2012",
    "Mazda Tribute SUV 2011",
    "McLaren MP4-12C Coupe 2012",
    "Mercedes-Benz 300-Class Convertible 1993",
    "Mercedes-Benz C-Class Sedan 2012",
    "Mercedes-Benz E-Class Sedan 2012",
    "Mercedes-Benz S-Class Sedan 2012",
    "Mercedes-Benz SL-Class Coupe 2009",
    "Mercedes-Benz Sprinter Van 2012",
    "Mitsubishi Lancer Sedan 2012",
    "Nissan 240SX Coupe 1998",
    "Nissan Juke Hatchback 2012",
    "Nissan Leaf Hatchback 2012",
    "Nissan NV Passenger Van 2012",
    "Plymouth Neon Coupe 1999",
    "Porsche Panamera Sedan 2012",
    "Ram C-V Cargo Van Minivan 2012",
    "Rolls-Royce Ghost Sedan 2012",
    "Rolls-Royce Phantom Drophead Coupe Convertible 2012",
    "Rolls-Royce Phantom Sedan 2012",
    "Scion xD Hatchback 2012",
    "Spyker C8 Convertible 2009",
    "Spyker C8 Coupe 2009",
    "Suzuki Aerio Sedan 2007",
    "Suzuki Kizashi Sedan 2012",
    "Suzuki SX4 Hatchback 2012",
    "Suzuki SX4 Sedan 2012",
    "Tesla Model S Sedan 2012",
    "Toyota 4Runner SUV 2012",
    "Toyota Camry Sedan 2012",
    "Toyota Corolla Sedan 2012",
    "Toyota Sequoia SUV 2012",
    "Volkswagen Beetle Hatchback 2012",
    "Volkswagen Golf Hatchback 1991",
    "Volkswagen Golf Hatchback 2012",
    "Volvo 240 Sedan 1993",
    "Volvo C30 Hatchback 2012",
    "Volvo XC90 SUV 2007",
    "smart fortwo Convertible 2012",
]

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# Load state_dict
state = torch.load(CarModel_MODEL_PATH, map_location=device)


def remove_prefix(text: str, prefix: str):
    if text.startswith(prefix):
        return text[len(prefix) :]
    return text


state = {
    remove_prefix(layer_name, "0.model."): weights
    for layer_name, weights in state.items()
}

# Instantiate the architecture
model = create_model(
    "tresnet_l.miil_in1k_448",
    pretrained=False,
    num_classes=len(VOCABULARY),
)
# Load weights
model.load_state_dict(state, strict=False)
model.to(device).eval()

# Define transforms
transform = T.Compose(
    [
        T.Resize((448, 448)),
        T.ToTensor(),
        T.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
    ],
)


def predict_car_model(image: Image.Image) -> tuple[str, float] | None:
    """
    Predicts the car model from an image using a PyTorch model.

    Parameters:
        image: PIL Image object containing the car

    Returns:
        Tuple of (model_name, confidence) or None if prediction fails or confidence < 0.3
    """
    try:
        # Prepare image
        img_t = transform(image.convert("RGB")).unsqueeze(0).to(device)
        # Inference
        with torch.no_grad():
            logits = model(img_t)
            probs = torch.softmax(logits, dim=1).squeeze(0)
            confidence, idx = probs.max(0)
            confidence = float(confidence)
        # Threshold check
        print(confidence, idx)
        if confidence < 0.3:
            return None
        # Map to class name
        return VOCABULARY[idx], confidence

    except Exception as e:
        print(f"Error during car model prediction: {e}")
        return None
