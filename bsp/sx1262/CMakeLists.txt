# STATIC: generates a complete and self-contained .a because it integrates
# both the HAL implementation (sx126x_hal.c) and the generic driver objects
# (via OBJECT). The linker receives a single .a with no unresolved external references.
add_library(bsp_sx1262 STATIC)

target_sources(bsp_sx1262 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x_hal_impl/sx126x_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sx1262_board.c
)

# PUBLIC: sx1262_board.h and sx126x_hal_context.h must be visible
# to any target that uses this BSP
target_include_directories(bsp_sx1262 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x_hal_impl
)

# PUBLIC in sx126x_driver: propagates its headers (sx126x.h,
# sx126x_hal.h) to anything that depends on bsp_sx1262_driver.
target_link_libraries(bsp_sx1262 PUBLIC
    sx126x_driver
    stm32cubemx
)