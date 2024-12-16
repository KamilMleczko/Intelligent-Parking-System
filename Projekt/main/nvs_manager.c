#include <stdbool.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "utils.h"


bool read_wifi_credentials_from_nvs(wifi_sta_config_t* wifi_config) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Ensure NVS is initialized
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("NVS", "NVS partition is truncated or requires an update");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to initialize NVS: %s", esp_err_to_name(err));
        return false;
    }

    // Open the NVS namespace used by WiFi
    err = nvs_open("nvs.net80211", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error opening NVS: %s", esp_err_to_name(err));
        return false;
    }

    // Read the stored credentials
    size_t required_size = sizeof(wifi_sta_config_t);
    err = nvs_get_blob(nvs_handle, "sta.ssid", wifi_config, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW("NVS", "WiFi credentials not found in NVS");
    } else if (err != ESP_OK) {
        ESP_LOGE("NVS", "Error reading WiFi credentials: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI("NVS", "WiFi credentials successfully retrieved");
    }

    // Close NVS handle
    nvs_close(nvs_handle);

    return (err == ESP_OK);
}
