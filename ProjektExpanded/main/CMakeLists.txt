# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")
idf_component_register(
    SRCS 
        main.c
        utils.c
        wifi.c
        my_ssd1306_init.c
        my_ssd1306.c
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
