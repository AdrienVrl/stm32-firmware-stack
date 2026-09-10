add_library(ml STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/network.c
                      ${CMAKE_CURRENT_SOURCE_DIR}/src/network_data.c)

target_include_directories(ml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(stedgeai_runtime STATIC IMPORTED)

set(ML_RUNTIME_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/NetworkRuntime1201_CM4_GCC.a)
target_link_libraries(ml PUBLIC ${ML_RUNTIME_LIB})
target_link_libraries(ml PUBLIC stedgeai_runtime)

target_compile_options(ml PRIVATE -Wno-error -Wno-pedantic
                                  -Wno-unused-parameter)
