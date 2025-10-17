cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_SOURCE_DIR}/../common/mesh_manager"
    "${CMAKE_SOURCE_DIR}/../common/mesh_protocol"
    "${CMAKE_SOURCE_DIR}/../common/node_config"
    "${CMAKE_SOURCE_DIR}/../common/sensor_base"
    "${CMAKE_SOURCE_DIR}/../common/actuator_base"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(node_ph_ec)

