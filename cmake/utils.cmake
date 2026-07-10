# cmake/utils.cmake

# ── Helper: add_subdirectory only if the module has landed ───────────────────
# Lets the root CMakeLists.txt reference modules (third-party deps, app,
# bootloader, ml, integration tests, ...) that don't exist in the repo yet
# without breaking configure for the modules that do.
function(add_subdirectory_if_exists dir)
  if(EXISTS ${CMAKE_SOURCE_DIR}/${dir}/CMakeLists.txt)
    add_subdirectory(${dir})
  else()
    message(STATUS "Skipping ${dir} (no CMakeLists.txt yet)")
  endif()
endfunction()

# ── Helper: register a `docs` target that runs Doxygen ────────────────────────
# Usage: add_docs_target()  (call once from the root CMakeLists.txt)
function(add_docs_target)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    add_custom_target(
      docs
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Generating documentation with Doxygen")
  else()
    message(STATUS "Doxygen not found — 'docs' target will not be available")
  endif()
endfunction()

# ── Helper: generate .bin and .hex from an ELF target ────────────────────────
# Usage: arm_generate_binary(firmware)
function(arm_generate_binary target)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}>
            ${CMAKE_BINARY_DIR}/${target}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target}>
            ${CMAKE_BINARY_DIR}/${target}.hex
    COMMENT "Generating ${target}.bin and ${target}.hex")
endfunction()

# ── Helper: print flash and RAM usage after build ────────────────────────────
# Usage: arm_print_size(firmware)
function(arm_print_size target)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${target}>
    COMMENT "Binary size for ${target}")
endfunction()

# ── Helper: register an OpenOCD GDB-server debug target ───────────────────────
# Usage: arm_add_debug_target(firmware)
function(arm_add_debug_target target)
  add_custom_target(
    debug_${target}
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
    COMMENT "Starting OpenOCD GDB server for ${target} on port 3333")
endfunction()

# ── Helper: generate disassembly listing ─────────────────────────────────────
# Usage: arm_disassemble(firmware)
function(arm_disassemble target)
  add_custom_target(
    ${target}_disasm
    COMMAND ${CMAKE_OBJDUMP} -d -S $<TARGET_FILE:${target}> >
            ${CMAKE_BINARY_DIR}/${target}.disasm
    DEPENDS ${target}
    COMMENT "Generating disassembly for ${target}")
endfunction()

# ── Helper: flash a binary via OpenOCD ───────────────────────────────────────
# Usage: arm_add_flash_target(firmware 0x08000000)
function(arm_add_flash_target target flash_address)
  add_custom_target(
    flash_${target}
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c
            "program ${CMAKE_BINARY_DIR}/${target}.bin
                verify reset exit ${flash_address}"
    DEPENDS ${target}
    COMMENT "Flashing ${target} to ${flash_address}")
endfunction()

# ── Helper: bundle all post-build steps together ─────────────────────────────
# Usage: arm_finalise_target(firmware 0x08000000) Runs generate_binary +
# print_size + disassemble + flash target in one call
function(arm_finalise_target target flash_address)
  arm_generate_binary(${target})
  arm_print_size(${target})
  arm_disassemble(${target})
  arm_add_flash_target(${target} ${flash_address})
  arm_add_debug_target(${target})
endfunction()

# ── Helper: create a firmware executable with standard flags ─────────────────
# Usage: arm_add_firmware( NAME        firmware SOURCES     src/main.c
# src/tasks/sensor_task.c LINKER ${CMAKE_SOURCE_DIR}/core/linker/application.ld
# FLASH_ADDR  0x08008000 LIBS bsp freertos )
function(arm_add_firmware)
  cmake_parse_arguments(
    ARG # prefix for parsed variables
    "" # options (boolean flags, no value)
    "NAME;LINKER;FLASH_ADDR" # single-value keywords
    "SOURCES;LIBS" # multi-value keywords
    ${ARGN} # arguments passed to the function
  )

  add_executable(${ARG_NAME} ${ARG_SOURCES})

  target_link_libraries(${ARG_NAME} PRIVATE ${ARG_LIBS})

  target_link_options(${ARG_NAME} PRIVATE -T${ARG_LINKER}
                      -Wl,-Map=${CMAKE_BINARY_DIR}/${ARG_NAME}.map)

  set_target_properties(${ARG_NAME} PROPERTIES LINK_DEPENDS ${ARG_LINKER})

  arm_finalise_target(${ARG_NAME} ${ARG_FLASH_ADDR})
endfunction()

# ── Helper: create a static library with standard settings ───────────────────
# Usage: arm_add_library( NAME    bsp SOURCES src/gpio.c src/uart.c INCLUDE
# include/ )
function(arm_add_library)
  cmake_parse_arguments(ARG "" "NAME;INCLUDE" "SOURCES" ${ARGN})

  add_library(${ARG_NAME} STATIC ${ARG_SOURCES})

  target_include_directories(${ARG_NAME} PUBLIC ${ARG_INCLUDE})
endfunction()

# ── Helper: register a Unity unit test ───────────────────────────────────────
# Usage: arm_add_unit_test( NAME    test_ring_buffer SOURCES
# tests/unit/test_ring_buffer.c LIBS    ring_buffer_lib )
function(arm_add_unit_test)
  cmake_parse_arguments(ARG "" "NAME" "SOURCES;LIBS" ${ARGN})

  add_executable(${ARG_NAME} ${ARG_SOURCES})

  target_link_libraries(${ARG_NAME} PRIVATE unity # Unity test framework
                                            ${ARG_LIBS})

  # Register with CTest so `ctest` runs all unit tests automatically
  add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})
endfunction()
