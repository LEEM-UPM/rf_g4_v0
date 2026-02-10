cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME rf_g4_v0)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})

add_subdirectory(cmake/stm32cubemx)
add_subdirectory(third_party/sx126x_driver)
add_subdirectory(bsp)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    poc/sx1262_tx/main.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)

target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    bsp_sx1262_driver
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
