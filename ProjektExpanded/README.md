# ESP-IDF template app

In order to provide wifi / mqtt server credentials, create a `main/credentials.h` file.
This file shall look like so:

```C
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_SSID ""
#define WIFI_PASS ""
#define WIFI_PROV_POP "abcd1234"
#define MQTT_BROKER_URI ""
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
// mqtt topic schema: organisation/location/spotId/action.
#define MQTT_TOPIC_PREFIX "home/garage/0/"
#define SERVER_URL "http://{PC running backend IP address}:8000/upload_picture/"
#endif
```

In order to provide mqtt config for kotlin client fill
`sensor-mqtt/config/mqtt.properties` according to specification in `sensor-mqtt/config/mqtt.example.properties`.

### Set up Camera
1. Start wifi router and connect it to your PC
2. Start backend server
3. Set up in ```credentails.h``` the IP address of your PC running the server (you can check it with ipconfig)
4. Start ESP32 
5. Connect ESP32 to your wifi router via mobile app
6. After provisioning ends camera will start taking pictures and sending them to server that will save them in input folder