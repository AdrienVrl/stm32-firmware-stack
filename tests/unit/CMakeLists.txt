# tests/unit/CMakeLists.txt

# ── Mock register definitions ─────────────────────────────────────────────────
# Shared across all test executables — provides fake STM32 register structs.
# Every test below depends on this, so the whole file is a no-op until
# mock_registers.c lands; tests then register themselves automatically.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mock_registers.c)

  add_library(mock_registers STATIC mock_registers.c)
  target_include_directories(mock_registers PUBLIC .)
  target_compile_definitions(mock_registers PUBLIC UNIT_TEST)

  # ── Helper function to reduce boilerplate
  # ───────────────────────────────────── Creates a test executable, links Unity
  # + CMock, registers with CTest
  function(add_unit_test)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;LIBS" ${ARGN})

    add_executable(${ARG_NAME} ${ARG_SOURCES})

    target_include_directories(
      ${ARG_NAME}
      PRIVATE ${CMAKE_SOURCE_DIR}/bsp/include
              ${CMAKE_SOURCE_DIR}/app/include
              ${CMAKE_SOURCE_DIR}/ml/include
              ${CMAKE_SOURCE_DIR}/core/include
              ${CMAKE_CURRENT_SOURCE_DIR} # for mock_registers.h
              ${CMAKE_CURRENT_SOURCE_DIR}/mocks # for generated CMock headers
    )

    target_compile_definitions(${ARG_NAME} PRIVATE UNIT_TEST)

    target_link_libraries(${ARG_NAME} PRIVATE unity cmock mock_registers
                                              ${ARG_LIBS})

    # Register with CTest — runs via: ctest --test-dir build/host
    add_test(NAME ${ARG_NAME} COMMAND ${ARG_NAME})

    # Print test output immediately on failure
    set_tests_properties(${ARG_NAME} PROPERTIES FAIL_REGULAR_EXPRESSION "FAIL")
  endfunction()

  # ── Test executables
  # ─────────────────────────────────────────────────────────── Each guarded
  # individually so tests come online one at a time as their driver + test
  # source files are written, instead of all-or-nothing.

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_ring_buffer.c)
    add_unit_test(NAME test_ring_buffer SOURCES test_ring_buffer.c
                  ${CMAKE_SOURCE_DIR}/bsp/src/ring_buffer.c)
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_gpio.c)
    add_unit_test(NAME test_gpio SOURCES test_gpio.c
                  ${CMAKE_SOURCE_DIR}/bsp/src/gpio.c mocks/mock_registers.c)
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_uart.c)
    add_unit_test(NAME test_uart SOURCES test_uart.c
                  ${CMAKE_SOURCE_DIR}/bsp/src/uart.c mocks/mock_registers.c)
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_mel_frontend.c)
    add_unit_test(NAME test_mel_frontend SOURCES test_mel_frontend.c
                  ${CMAKE_SOURCE_DIR}/ml/src/mel_frontend.c)
  endif()

else()
  message(
    STATUS "tests/unit: no test sources yet — skipping unit test registration")
endif()
