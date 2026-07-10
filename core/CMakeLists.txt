# core/CMakeLists.txt

add_library(core INTERFACE)
target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# assert_handler.c depends on CMSIS intrinsics and bsp/include/uart.h, so it
# only gets compiled once an ARM executable actually links against `core`.
if(TARGET_PLATFORM STREQUAL "arm")
  target_sources(core
                 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src/assert_handler.c)
endif()
