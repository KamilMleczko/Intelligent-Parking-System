ESP-IDF template app
====================

In order to provide wifi / mqtt server credentials, create a `main/credentials.h` file.
This file shall look like so:

```C
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_SSID ""
#define WIFI_PASS ""
#define MQTT_BROKER_URI ""
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
// mqtt topic schema: organisation/location/spotId/action.
#define MQTT_TOPIC_PREFIX "home/garage/0/"

#endif
```

In order to provide mqtt config for kotlin client fill
`sensor-mqtt/config/mqtt.properties` according to specification in `sensor-mqtt/config/mqtt.example.properties`.
