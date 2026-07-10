# .gdbinit
# Project-local GDB init for STM32F446RE on Nucleo board.
# Requires OpenOCD to be running in a separate terminal:
#   cmake --build build --target debug_firmware

# ── 1. Connect to OpenOCD ─────────────────────────────────────────────────────
# OpenOCD exposes a GDB server on port 3333 by default.
target extended-remote :3333

# ── 2. Load symbols from the ELF ─────────────────────────────────────────────
# This loads debug symbols without re-flashing the binary.
# Change the path to whichever target you're debugging.
file build/arm/firmware.elf

# ── 3. Reset and halt the CPU before doing anything ──────────────────────────
# 'monitor' sends a command directly to OpenOCD rather than GDB.
monitor reset halt

# ── 4. Breakpoints on fault handlers ─────────────────────────────────────────
# These fire automatically on any hard fault, stack overflow,
# or malloc failure — saving you from hunting for them manually.
break HardFault_Handler
break vApplicationStackOverflowHook
break vApplicationMallocFailedHook

# ── 5. Breakpoint at main ─────────────────────────────────────────────────────
# Halts at the first line of main() so you start in a known state.
break main

# ── 6. Pretty-printing ───────────────────────────────────────────────────────
# Print structures recursively with field names rather than raw hex.
set print pretty on

# Show array contents inline rather than as a pointer address.
set print array on
set print array-indexes on

# Show the full string content of char* rather than just the address.
set print null-terminated-string on

# ── 7. History ────────────────────────────────────────────────────────────────
# Save command history across sessions so you can arrow-up previous commands.
set history save on
set history filename ~/.gdb_history
set history size 1000

# ── 8. Disassembly flavor ─────────────────────────────────────────────────────
# Use Intel syntax — easier to read than AT&T for most people.
# For ARM this mostly affects how immediate values are displayed.
set disassembly-flavor intel

# ── 9. Confirm dangerous operations ──────────────────────────────────────────
# Asks for confirmation before doing things like deleting all breakpoints.
set confirm on

# ── 10. Run to main ──────────────────────────────────────────────────────────
# Start execution — GDB will halt at the 'break main' set above.
continue
