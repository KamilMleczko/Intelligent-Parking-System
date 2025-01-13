#include "utils.h"

#include <stdio.h>

#include "wifi.h"

void write_device_name(char* buffer) {
  uint8_t eth_mac[6];
  const char* ssid_prefix = "DOOR_";
  esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

  if (DEVICE_NAME_LEN >= 12) {
    snprintf(buffer, DEVICE_NAME_LEN, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
             eth_mac[4], eth_mac[5]);
  } else {
    // never happens, but the compiler's happy :D
    ESP_LOGE("DEVICE_NAME", "DEVICE_NAME_LEN too small, buffer truncated.");
    snprintf(buffer, DEVICE_NAME_LEN, "%s",
             ssid_prefix);  // Write only the prefix
  }
}
char DEVICE_NAME_BUFFER[DEVICE_NAME_LEN];
char* DEVICE_NAME = DEVICE_NAME_BUFFER;
void init_device_name(void) { write_device_name(DEVICE_NAME_BUFFER); }
