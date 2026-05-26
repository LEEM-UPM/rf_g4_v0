# platform/stm32/sx126x_driver/CMakeLists.txt
#
# STM32 platform implementation of the sx126x HAL.
#
# Provides the four functions left unresolved by sx126x_driver (OBJECT):
#   sx126x_hal_reset()
#   sx126x_hal_wakeup()
#   sx126x_hal_write()
#   sx126x_hal_read()
#
# Also exposes sx126x_hal_context_stm32.h, which defines the concrete
# context struct (SPI_HandleTypeDef*, GPIO ports/pins). This header is
# included by bsp/sx1262/bsp_sx1262.c to build the context instance,
# and by sx126x_hal.c to cast the opaque void* back to the concrete type.

add_library(platform_sx126x_stm32 STATIC)

target_sources(platform_sx126x_stm32 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x_hal.c
)

target_include_directories(platform_sx126x_stm32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(platform_sx126x_stm32 PUBLIC
    sx126x_driver
    stm32cubemx
)