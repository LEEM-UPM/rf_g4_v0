# bsp/can/CMakeLists.txt
#
# Board-specific CAN1 (FDCAN1) BSP for the CAN<->LoRa bridge.
#
# Wraps the FDCAN1 HAL handle (hfdcan1, from Core/Src/fdcan.c) behind a
# plain uint32_t/byte-buffer API (bsp_can1_start/poll_rx/send).
#
# Unlike bsp_sx1262, there is no external chip driver to link here: FDCAN
# is a built-in STM32 peripheral, so the STM32Cube HAL itself plays the
# role that platform_sx126x_stm32 plays for the SX1262 (chip driver + HAL
# glue in one). RX is polling-based, so there is no EXTI callback to
# register with bsp_board either — bsp_can1 does not depend on bsp_core.

add_library(bsp_can1 STATIC)

target_sources(bsp_can1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_can1.c
)

# PUBLIC: bsp_can1.h must be visible to the application and PoCs.
target_include_directories(bsp_can1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bsp_can1 PUBLIC
    stm32cubemx
)
