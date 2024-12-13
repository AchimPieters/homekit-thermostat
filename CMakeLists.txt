cmake_minimum_required(VERSION 3.5)

set(PARTITION_TABLE_DEFAULT "partitions.csv" CACHE FILE "Default partition table file")
set(EXTRA_COMPONENT_DIRS ./components)
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(main)
