# third_party/sx126x_driver/CMakeLists.txt
#
# Semtech sx126x generic radio driver — unmodified external code.
#
# This target is completely platform-agnostic. It knows how to encode
# every command the SX126x chip understands, but it has zero knowledge
# of the hardware it runs on. It declares four functions that it calls
# but does NOT implement:
#
#   sx126x_hal_reset()
#   sx126x_hal_wakeup()
#   sx126x_hal_write()
#   sx126x_hal_read()
#
# These symbols are left unresolved intentionally. The correct
# implementation is selected by the build system depending on the target:
#
#   Embedded (STM32):  platform/stm32/sx126x_driver/  → platform_sx126x_stm32
#   Host (tests):      platform/test/sx126x_driver/   → platform_sx126x_test
#
# Why OBJECT and not STATIC?
#
# With STATIC, the linker extracts only the object files it needs from
# the archive. If it does not find a consumer of sx126x_hal_write() at
# archive-extraction time, it may not include the object that calls it,
# causing the symbol to go unresolved silently. With OBJECT, all compiled
# translation units are handed directly to the final link step, where the
# linker sees both the calls (from sx126x.o) and the definitions (from
# sx126x_hal.o in the platform target) at the same time and resolves them
# correctly.
#
# This file must never be modified. If a bug is found in the Semtech driver,
# it must be patched in the platform or driver_sx1262 layer, not here.

add_library(sx126x_driver OBJECT)

target_sources(sx126x_driver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sx126x_driver_version.c
)

target_include_directories(sx126x_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)