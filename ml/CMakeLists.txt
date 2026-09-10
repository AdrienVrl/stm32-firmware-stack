set(ML_RUNTIME_LIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/NetworkRuntime1201_CM4_GCC.a)

add_library(ml STATIC src/network.c src/network_data.c)
target_include_directories(ml PUBLIC include)
target_link_libraries(ml PUBLIC ${ML_RUNTIME_LIB})
target_compile_options(ml PRIVATE -Wno-error)
