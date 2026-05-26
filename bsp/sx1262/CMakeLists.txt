# bsp/sx1262/CMakeLists.txt
#
# Board-specific SX1262 BSP for the RF board.
#
# This target provides two things:
#
#   1. bsp_sx1262.c — constructs the sx1262_config_t with the real pin
#      assignments, RF switch callbacks, and IRQ registration hook for
#      this specific PCB revision.
#
#   2. Satisfies the context type dependency: bsp_sx1262.c needs
#      sx126x_hal_context_stm32.h to build the concrete HAL context.
#      That header comes from platform_sx126x_stm32 via PUBLIC propagation.
#
# What this target does not contain:
#   sx126x_hal.c lives in platform/stm32/sx126x_driver/.
#   bsp_sx1262 does not implement the Semtech HAL — that is the platform's
#   responsibility. bsp_sx1262 only knows about THIS board's wiring.

add_library(bsp_sx1262 STATIC)

target_sources(bsp_sx1262 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_sx1262.c
)

# PUBLIC: bsp_sx1262.h must be visible to the application and PoCs.
target_include_directories(bsp_sx1262 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bsp_sx1262 PUBLIC
    driver_sx1262
    platform_sx126x_stm32
    stm32cubemx
PRIVATE
    bsp_core # provides board_internal.h, not propagated upward
)