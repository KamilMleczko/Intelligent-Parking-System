#ifndef UTILS_H
#define UTILS_H

#define LOG_HW "[HARDWARE] "
#define LOG_WIFI "[WIFI] "
#define LOG_PROV "[WIFI PROVISIONING] "
#define LOG_DISTANCE "[DISTANCE MEASUREMENTS]  "
#define LOG_MQTT "[MQTT] "
#define LOG_NVS "[NVS] "

#define DEVICE_NAME_LEN 12
void write_device_name(char* buffer);

#endif  // UTILS_H
