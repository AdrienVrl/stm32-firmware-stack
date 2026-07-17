/******************************************************************************
 * main.c
 * Minimal smoke test: verifies Reset_Handler correctly copied .data from
 * Flash and zero-filled .bss before main() was called.
 *
 * - g_initialized lives in .data: nonzero, so it only reads back correctly
 *   if the Flash->SRAM copy in Reset_Handler actually ran.
 * - g_uninitialized lives in .bss: implicitly zero, so it only reads back
 *   as 0 if the zero-fill loop actually ran (uninitialized SRAM is NOT
 *   guaranteed to be zero at power-on -- it may contain leftover garbage
 *   from a previous run, or arbitrary reset-state noise).
 *
 * Both reads go through volatile pointers so the compiler can't prove the
 * values are compile-time constants and optimize the reads (or the whole
 * variables) away entirely.
 *****************************************************************************/

#include <stdint.h>

/* Nonzero initializer -> placed in .data (Flash-resident init value,
 * SRAM-resident runtime location). */
uint32_t g_initialized = 0xCAFEBABEu;

/* No initializer -> placed in .bss (zero-filled at runtime, no Flash
 * footprint for its value). */
uint32_t g_uninitialized;

int main(void)
{
    volatile uint32_t *p_init   = &g_initialized;
    volatile uint32_t *p_uninit = &g_uninitialized;

    /* Force real reads. Since the pointers are volatile-qualified, the
     * compiler must emit actual load instructions here and cannot
     * constant-fold or discard these accesses, even though nothing
     * "uses" the results afterward. */
    uint32_t read_init   = *p_init;
    uint32_t read_uninit = *p_uninit;

    /* Park the values somewhere a debugger can inspect post-mortem
     * (register view or by reading these statics directly), and give
     * the optimizer one more reason not to drop the reads above.
     *
     * These are deliberately write-only from C's point of view -- their
     * whole purpose is to be read externally, by a debugger, after we've
     * halted below. GCC's -Wunused-but-set-variable doesn't know that,
     * so it's silenced specifically on these two variables rather than
     * disabling -Werror project-wide (which would also hide genuine
     * bugs elsewhere). */
    static volatile uint32_t debug_init __attribute__((unused));
    static volatile uint32_t debug_uninit __attribute__((unused));
    debug_init   = read_init;
    debug_uninit = read_uninit;

    while (1)
    {
        /* Halt here. Break in a debugger and check:
         *   g_initialized   == 0xCAFEBABE  (.data copy worked)
         *   g_uninitialized == 0x00000000  (.bss zero-fill worked)
         */
    }
}
