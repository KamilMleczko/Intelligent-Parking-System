# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS 
        main.c
        mqtt_utils.c
        wifi.c
        nvs_manager.c
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES 
        mqtt
        nvs_flash
        driver
        esp_wifi
        wifi_provisioning
        esp_netif
    PRIV_REQUIRES       # optional, list the private requirements
    EMBED_TXTFILES server_root_cert.pem
)
