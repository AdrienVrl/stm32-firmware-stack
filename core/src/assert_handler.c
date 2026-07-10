#include "common.h"
#include "uart.h" // or whatever your debug output is

// This symbol is defined in your linker script and marks the top of the stack.
// Declaring it here lets the handler print the stack pointer for debugging.
extern uint32_t _estack;

/**
 * @brief Called by ASSERT() macro on assertion failure.
 *
 * Prints the file and line number over UART then halts in an infinite loop.
 * Attach a debugger to inspect the call stack at the point of failure.
 *
 * This function is marked noreturn so the compiler knows it never returns,
 * which avoids spurious "missing return" warnings in callers.
 */
__attribute__((noreturn)) void assert_failed(const char *file, uint32_t line)
{
    // Disable interrupts — we're in an unrecoverable state,
    // don't let an ISR run and complicate the debug picture.
    __disable_irq();

    // Print the assertion location over UART.
    // If UART isn't initialised yet, this is a no-op — attach a debugger.
    // Using a simple write here rather than printf to avoid
    // any risk of printf itself asserting.
    uart_write_str("\r\n\r\n*** ASSERTION FAILED ***\r\n");
    uart_write_str("File: ");
    uart_write_str(file);
    uart_write_str("\r\nLine: ");
    uart_write_u32(line);
    uart_write_str("\r\n");

    // Halt. A debugger attached here will show the exact call stack
    // that led to the assertion failure via 'bt' in GDB.
    while (1)
    {
        __asm volatile("nop");
    }
}
