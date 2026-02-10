add_library(bsp_sx1262_driver STATIC)

target_sources(bsp_sx1262_driver PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/sx1262/sx126x_hal.c
)

target_include_directories(bsp_sx1262_driver PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/sx1262
)

target_link_libraries(bsp_sx1262_driver PUBLIC 
    sx126x_driver
    stm32cubemx
)