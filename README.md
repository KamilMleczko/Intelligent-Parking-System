# Intelligent-Parking-System

## Założenia Projektu
Inteligentny system parkingowy to urządzenie IoT służące do zliczania i wyświetlania aktualnej liczby samochodów na parkingu. Bramka wjazdowa znajduje się po jednej stronie, a wyjazdowa po drugiej (konfigurację można zmienić). Do urządzenia dołączona jest dedykowana aplikacja mobilna zapewniająca łatwą i szybką konfigurację urządzenia wraz z analizą zapełnienia parkingu w czasie. 

### Architektura:
Projekt składa się z:
- **Oprogramowania licznika** (ESP32)
- **Aplikacji serwerowej** (Kotlin)
- **Aplikacji mobilnej** (Kotlin, Jetpack Compose)

### Wykorzystane Elementy
Konstrukcja obejmuje:
- **Płytkę ESP32** (mikrokontroler sterujący)
- **Dwa sensory odległości HC-SR04** (wykrywają samochody)
- **Buzzer** (sygnalizacja dźwiękowa)
- **Wyświetlacz OLED 128x64 (I2C)** (wyświetlanie informacji o aktualnym zapełnieniu parkingu)

### Funkcjonalności:
- Zliczanie aktualnej liczby zajętych miejsc na parkingu
- Emisja dźwięku przy wjeździe i wyjeździe samochodu.
- Wysyłanie powiadomień na serwer przy użyciu MQTT
- Przechowywanie danych o stanie parkingu w czasie w firebase.
- Konfigurowalny limit maksymalnej liczby miejsc na parkingu (domyślnie 30).
- Konfiguracja WIFI za pomocą BLE
- Blokada bramki wjazdowej po osiągnięciu limitu.
- Aplikacja mobilna do zarządzania wieloma urządzeniami jednocześnie i analizy danych w czasie rzeczywistym
