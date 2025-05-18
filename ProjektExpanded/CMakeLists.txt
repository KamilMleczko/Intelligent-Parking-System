cmake_minimum_required(VERSION 3.5)

set(IDF_FIRMWARE_PARTITIONS_FLASH_SPIFFS yes)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(app-template)

# 2) tell CMake how to bake our SPIFFS folder into spiffs.bin
spiffs_create_partition_image(
    spiffs
    spiffs
    0x100000
)
