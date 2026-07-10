# tests/CMakeLists.txt

# ── Unity static library ──────────────────────────────────────────────────────
add_library(unity STATIC unity/src/unity.c)
target_include_directories(unity PUBLIC unity/src)

# ── CMock static library ──────────────────────────────────────────────────────
add_library(cmock STATIC cmock/src/cmock.c cexception/lib/CException.c)
target_include_directories(cmock PUBLIC cmock/src cexception/lib)
target_link_libraries(cmock PUBLIC unity)

add_subdirectory(unit)
