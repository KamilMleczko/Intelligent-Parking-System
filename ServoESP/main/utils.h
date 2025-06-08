#ifndef UTILS_H
#define UTILS_H

#define LOG_HW "[HARDWARE] "
#define LOG_WIFI "[WIFI] "
#define LOG_PROV "[WIFI PROVISIONING] "
#define LOG_DISTANCE "[DISTANCE MEASUREMENTS]  "
#define LOG_MQTT "[MQTT] "
#define LOG_NVS "[NVS] "

#define DEVICE_NAME_LEN 12

extern char* DEVICE_NAME;
void init_device_name(void);
void write_device_name(char* buffer);

#endif  // UTILS_H
