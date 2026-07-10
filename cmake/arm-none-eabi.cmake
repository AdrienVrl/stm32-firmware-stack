# cmake/arm-none-eabi.cmake

# ── 1. Tell CMake this is a cross-compilation build ──────────────────────────
# Without this, CMake assumes host == target and tries to run compiled binaries
# during compiler detection, which fails for ARM binaries on an x86 host.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ── 2. Locate the toolchain ───────────────────────────────────────────────────
# Find the compiler on PATH. If you installed arm-none-eabi-gcc via apt or
# Homebrew it will be found automatically. You can also hardcode the path:
# set(TOOLCHAIN_PREFIX /usr/bin/arm-none-eabi-)
set(TOOLCHAIN_PREFIX arm-none-eabi-)

find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++ REQUIRED)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc REQUIRED)
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}ar REQUIRED)
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy REQUIRED)
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump REQUIRED)
find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size REQUIRED)

# ── 3. CPU flags ──────────────────────────────────────────────────────────────
# These are specific to the STM32F446RE (Cortex-M4 with FPU): -mcpu=cortex-m4 :
# generate Cortex-M4 instructions -mthumb                : use Thumb-2
# instruction set (required for Cortex-M) -mfpu=fpv4-sp-d16      :
# single-precision FPU with 16 double-precision regs -mfloat-abi=hard       :
# use FPU registers for float args (ABI must match every object file and library
# you link against)
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# ── 4. Compiler flags ─────────────────────────────────────────────────────────
# These apply to all build types. Build-type-specific flags (debug/release) are
# set separately below. -fdata-sections -ffunction-sections : place each symbol
# in its own section so the linker can dead-strip unused code and data via
# --gc-sections -fno-common                         : don't merge uninitialized
# globals (catches duplicate symbol bugs) -Wall -Wextra                       :
# enable warnings -Wpedantic                          : enforce strict C
# standard compliance -Werror                             : treat warnings as
# errors (enforce this from day one, don't add it later)
set(COMMON_FLAGS
    "${CPU_FLAGS} \
    -fdata-sections \
    -ffunction-sections \
    -fno-common \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Werror")

set(CMAKE_C_FLAGS
    "${COMMON_FLAGS}"
    CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS
    "${COMMON_FLAGS} -x assembler-with-cpp"
    CACHE STRING "" FORCE)

# ── 5. Build-type flags ───────────────────────────────────────────────────────
# Debug: no optimization, full debug info, enable assertions Release: optimize
# for size (-Os is usually better than -O2 for embedded), strip debug info,
# disable assertions
set(CMAKE_C_FLAGS_DEBUG
    "-Og -g3 -DDEBUG"
    CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE
    "-Os -DNDEBUG"
    CACHE STRING "" FORCE)

# ── 6. Linker flags ───────────────────────────────────────────────────────────
# -T${LINKER_SCRIPT}    : linker script path (set in root CMakeLists.txt)
# --gc-sections         : remove unused sections (works with
# -ffunction-sections) -lc -lm -lnosys       : link against newlib C library,
# math library, and nosys stub (provides minimal syscall stubs so you don't need
# to implement all of them) --specs=nano.specs    : use newlib-nano (smaller
# footprint than full newlib) --specs=nosys.specs   : use nosys stubs
# (alternative to -lnosys) -Wl,--print-memory-usage : print flash/RAM usage
# after linking (very useful) -Wl,-Map=${PROJECT_NAME}.map : generate a map file
# showing where every symbol ended up in memory
set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS} \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,-Map=${PROJECT_NAME}.map \
    --specs=nano.specs \
    --specs=nosys.specs \
    -lc -lm"
    CACHE STRING "" FORCE)

# ── 7. Prevent CMake from testing the compiler ────────────────────────────────
# CMake tries to compile and run a test binary to verify the compiler works.
# Running an ARM binary on x86 fails, so we tell it to only compile, not run.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ── 8. Search path configuration ─────────────────────────────────────────────
# Tell CMake not to look for libraries or headers in host system paths.
# CMAKE_FIND_ROOT_PATH_MODE_* ONLY means: only search inside the toolchain
# sysroot, never in host system directories.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) # but find programs on host PATH
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
