# CMakeLists.txt (project root)
#
# Central build definition for the LEEM RF board firmware.
cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for clangd / IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME rf_g4_v0)
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C CXX ASM)

# ---------------------------------------------------------------------------
# IMPORTANT: the executable must be created BEFORE including the CubeMX
# glue file. cmake/stm32cubemx/CMakeLists.txt references ${CMAKE_PROJECT_NAME}
# with target_sources() and target_link_libraries(), so the target must
# already exist when those commands run.
# ---------------------------------------------------------------------------
add_executable(${CMAKE_PROJECT_NAME})
 
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(utils)
add_subdirectory(third_party/sx126x_driver)
add_subdirectory(platform/stm32/sx126x_driver) 
add_subdirectory(drivers/sx1262)
add_subdirectory(bsp)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    #poc/blinking_led/main.c
    #poc/sx1262/chip_status.c
    poc/sx1262/tx_rx.c
    #poc/aplication/app.c
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    bsp_board
)

# Workaround: remove spurious 'ob' from implicit link libraries when
# mixing C and C++ translation units with arm-none-eabi-gcc.
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)