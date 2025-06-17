# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")
idf_component_register(
    SRCS 
        main.c
        utils.c
        wifi.c
        camera_stream.c
        ultrasonic.c
    INCLUDE_DIRS        # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES 
        nvs_flash
        driver
        esp_wifi
        wifi_provisioning
        esp_netif
        esp-tls
        spiffs              # for esp_vfs_spiffs_register()
        esp_http_server     # for httpd_start() and URI handlers
        esp_websocket_client
        json
    PRIV_REQUIRES       # optional, list the private requirements
    EMBED_TXTFILES server_root_cert.pem
)
