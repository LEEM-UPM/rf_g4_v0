# bsp/CMakeLists.txt
#
# Top-level BSP target. Owns two responsibilities:
#   1. board.c — single entry point for MCU peripheral initialisation
#      and EXTI dispatch for the entire board.
#   2. bsp_core — header-only target that exposes BSP-internal headers
#      (board_internal.h) to all chip BSP modules without creating
#      circular dependencies.
#
# Why bsp_core is defined before add_subdirectory(sx1262):
#   bsp_board links bsp_sx1262, and bsp_sx1262 needs board_internal.h.
#   If bsp_core were defined after add_subdirectory, it would not exist
#   yet when bsp_sx1262's CMakeLists.txt runs. Defining it first breaks
#   the circular include problem while keeping the include private to
#   the BSP layer.
#
# When a new chip BSP is added (e.g. bsp_sx1280), two steps are needed:
#   1. add_subdirectory(sx1280) here
#   2. target_link_libraries(bsp_board PUBLIC bsp_sx1280) here

add_library(bsp_core INTERFACE)

target_include_directories(bsp_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(sx1262)

add_library(bsp_board STATIC)

target_sources(bsp_board PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
)

target_include_directories(bsp_board PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bsp_board PUBLIC
    bsp_sx1262
    bsp_core
)