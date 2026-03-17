cmake_minimum_required(VERSION 3.22)

# OBJECT instead of STATIC because the driver is not self-contained:
# sx126x_hal_write/read/reset/wakeup are external symbols
# that must be implemented by the BSP. With OBJECT, its .o files are
# directly integrated into the consumer, avoiding dangling references.
add_library(sx126x_driver OBJECT)

target_sources(sx126x_driver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x_driver_version.c
)

# PUBLIC: sx126x.h and sx126x_hal.h are part of the driver's
# public contract; any target that depends on it
# must be able to include them
target_include_directories(sx126x_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)