# -------------------------------------------------------------------------
# app/CMakeLists.txt
#
# Defines the "firmware" target: the STM32F446xE application image.
# arm_add_firmware() itself is defined in cmake/utils.cmake and must already be
# include()'d by the top-level CMakeLists.txt before this subdirectory is
# processed (i.e. include(cmake/utils.cmake) must come before
# add_subdirectory(app) at the project root).
# -------------------------------------------------------------------------

arm_add_firmware(
  NAME
  firmware
  LINKER
  ${CMAKE_SOURCE_DIR}/core/linker/application.ld
  FLASH_ADDR
  0x08000000
  SOURCES
  ${CMAKE_SOURCE_DIR}/core/src/startup.c
  ${CMAKE_SOURCE_DIR}/core/src/system_stm32f4xx.c
  ${CMAKE_SOURCE_DIR}/app/src/main.c
  LIBS
  bsp
  INCLUDES
  ${CMAKE_SOURCE_DIR}/core/include)
