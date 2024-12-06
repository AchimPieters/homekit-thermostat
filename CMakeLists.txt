cmake_minimum_required(VERSION 3.5)

message(STATUS "Using partition table for ESP32C6")
set(PARTITION_TABLE_DEFAULT "partitions_esp32c6.csv" CACHE FILE "Default partition table file")
message(STATUS "Partition table file: ${PARTITION_TABLE_DEFAULT}")

set(EXTRA_COMPONENT_DIRS ./components)
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)
