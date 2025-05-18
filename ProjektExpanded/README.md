# ESP-IDF template app

In order to provide wifi / mqtt server credentials, create a `main/credentials.h` file.
This file shall look like so:

```C
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_SSID "SSID" //it is advised to use your own wifi over hotspot
#define WIFI_PASS "password"
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