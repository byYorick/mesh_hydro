cmake_minimum_required(VERSION 3.16)

# Добавление common компонентов
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_SOURCE_DIR}/../common"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node_ph_ec)
