# NODE Display - TFT Dashboard для Mesh Hydro System
cmake_minimum_required(VERSION 3.16)

# Добавление пути к common компонентам
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/../common"
    "${CMAKE_CURRENT_LIST_DIR}/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node_display)
