# drivers/sx1262/CMakeLists.txt
#
# Mid-level SX1262 driver.
#
# This target contains the chip-level logic: init sequence, LoRa radio
# config, TX/RX state machine, IRQ handling. It is board-agnostic: no
# STM32 HAL includes, no GPIO macros, no SPI handles appear in its sources.

add_library(driver_sx1262 STATIC)

target_sources(driver_sx1262 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sx1262.c
)

target_include_directories(driver_sx1262 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(driver_sx1262 PUBLIC
    sx126x_driver
    utils
)