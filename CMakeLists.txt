set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.20)

# Toolchain must be set before project()
if(NOT DEFINED TARGET_PLATFORM)
  set(TARGET_PLATFORM
      "arm"
      CACHE STRING "Build target: arm or host")
endif()

if(TARGET_PLATFORM STREQUAL "arm")
  set(CMAKE_TOOLCHAIN_FILE
      ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake
      CACHE STRING "" FORCE)
else()
  set(CMAKE_TOOLCHAIN_FILE
      ${CMAKE_SOURCE_DIR}/cmake/host.cmake
      CACHE STRING "" FORCE)
endif()

project(stm32_firmware C ASM)

enable_testing()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/utils.cmake)
add_docs_target()

# Internal modules
add_subdirectory(core)
add_subdirectory(bsp)

if(TARGET_PLATFORM STREQUAL "arm")
  add_library(freertos_config INTERFACE)
  target_include_directories(
    freertos_config INTERFACE ${CMAKE_SOURCE_DIR}/bsp/include
                              ${CMAKE_SOURCE_DIR}/core/include)
  set(FREERTOS_PORT
      "GCC_ARM_CM4F"
      CACHE STRING "" FORCE)
  set(FREERTOS_HEAP
      "4"
      CACHE STRING "" FORCE)
  # Third-party dependencies
  add_subdirectory_if_exists(third_party/FreeRTOS-Kernel)
  add_subdirectory_if_exists(third_party/CMSIS-DSP)

  add_subdirectory_if_exists(bootloader)
  add_subdirectory_if_exists(app)
  add_subdirectory_if_exists(ml)
  add_subdirectory_if_exists(tests/integration)
else()
  add_subdirectory(tests)
endif()
