cmake_minimum_required(VERSION 3.22)

add_library(sx126x_driver STATIC)

target_sources(sx126x_driver PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x_driver_version.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x.c
)

target_include_directories(sx126x_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)