# NODE pH - ESP32-C3
# Автономный узел управления pH

cmake_minimum_required(VERSION 3.16)

# Подключаем common компоненты
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/../common"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node_ph)

