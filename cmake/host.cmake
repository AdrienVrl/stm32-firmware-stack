# cmake/host.cmake

# ── 1. Target is the host machine ────────────────────────────────────────────
# We explicitly set this to remind CMake this is a native build, not a
# cross-compilation. This is the default but being explicit avoids confusion
# when both toolchain files exist in the project.
set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
set(CMAKE_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR})

# ── 2. Use the host compiler ──────────────────────────────────────────────────
# Let CMake find the system compiler automatically — don't hardcode gcc or clang
# so the file works on Linux, macOS, and Windows without modification. If you
# want to force a specific compiler you can set these explicitly:
# set(CMAKE_C_COMPILER gcc) set(CMAKE_C_COMPILER clang)
find_program(CMAKE_C_COMPILER NAMES gcc clang REQUIRED)
find_program(CMAKE_CXX_COMPILER NAMES g++ clang++ REQUIRED)

# ── 3. Compiler flags ─────────────────────────────────────────────────────────
# No CPU-specific flags here — those are ARM-only. -coverage enables gcov
# coverage instrumentation, useful for unit tests. Keep warnings consistent with
# the ARM build so the same code compiles cleanly under both toolchains.
set(CMAKE_C_FLAGS
    "-Wall -Wextra -Wpedantic -Werror -fprofile-arcs -ftest-coverage"
    CACHE STRING "" FORCE)

# ── 4. Build type flags ───────────────────────────────────────────────────────
set(CMAKE_C_FLAGS_DEBUG
    "-Og -g3 -DDEBUG"
    CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE
    "-O2 -DNDEBUG"
    CACHE STRING "" FORCE)

# ── 5. No cross-compilation search restrictions ───────────────────────────────
# Unlike the ARM toolchain, we want CMake to search host system paths normally
# for find_package, find_library, find_program.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
