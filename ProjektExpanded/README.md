# ESP-IDF template app

In order to provide wifi / mqtt server credentials, create a `main/credentials.h` file.
This file shall look like so:

```C
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_AP_SSID "5G_COSMIC_RAY_ATTRACTOR"
#define WIFI_AP_PASS "somePassword"
#define STREAM_SERVER_HOST "192.168.x.x" // to check your ip - run abckend server, 
                                        // then ins seperate terminal add 'ipconfig' and copy 'IPv4 Address'
#define STREAM_SERVER_PORT 8000 // don't change
#define STREAM_SERVER_PATH "/ws_stream/esp32cam1"  // Include a unique device ID, don't change
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

### SoftAP provisioning

We provision wifi credentials using SoftAP now.
To provision wifi connect to AP with name specified by `WIFI_AP_SSID` variable with
`WIFI_AP_PASS` variable from `credentials.h` file.
Then head to `parking.local` in the web browser (or 192.168.4.1, if there is problem with DNS resolution)

> [!WARNING]
> You need to first flash the spiffs, so that static html files are served correctly; `idf.py spiffs-flash`
