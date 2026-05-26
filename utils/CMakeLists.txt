# utils/CMakeLists.txt
#
# Header-only transversal utilities.
# INTERFACE: no sources to compile, only exposes the include path.
# Any target that links against utils gets access to all headers here.

add_library(utils INTERFACE)

target_include_directories(utils INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
)