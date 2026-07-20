#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ── Error / status type ───────────────────────────────────────────────────────
// Every function that can fail returns this type.
// Never use int or bool as a return type for error reporting.
typedef enum
{
    STATUS_OK              = 0, /**< Operation completed successfully. */
    STATUS_ERROR           = 1, /**< Generic unspecified error. */
    STATUS_TIMEOUT         = 2, /**< Operation timed out. */
    STATUS_BUSY            = 3, /**< Peripheral or resource is busy. */
    STATUS_INVALID_ARG     = 4, /**< A NULL or out-of-range argument was passed. */
    STATUS_BUFFER_FULL     = 5, /**< Ring buffer or queue has no space. */
    STATUS_BUFFER_EMPTY    = 6, /**< Ring buffer or queue has no data. */
    STATUS_NOT_INITIALIZED = 7, /**< Module was used before being initialised. */
    STATUS_HW_ERROR        = 8, /**< Hardware returned an unexpected state. */
} status_t;

// ── Assertion macros ──────────────────────────────────────────────────────────
// ASSERT() is for programmer errors — things that should never happen
// if the code is correct (null pointers, invalid config, etc.).
// It is NOT for runtime errors like timeouts or hardware failures —
// use STATUS_* return codes for those.
//
// In debug builds:  halts in an infinite loop with file/line info over UART.
// In release builds: disabled entirely (NDEBUG defined by CMake Release config).
//
// Usage:
//   ASSERT(handle != NULL);
//   ASSERT(config->baud_rate > 0);

#ifndef NDEBUG

// Forward declaration — implemented in assert_handler.c
void assert_failed(const char *file, uint32_t line);

#define ASSERT(expr)                                                                               \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            assert_failed(__FILE__, __LINE__);                                                     \
        }                                                                                          \
    } while (0)

#else
// Release build: assertions compile away to nothing
#define ASSERT(expr) ((void)(expr))
#endif

// ── Static assert ─────────────────────────────────────────────────────────────
// Compile-time assertion — fails at compile time, not runtime.
// Use for things you can verify at compile time:
//   - struct sizes
//   - buffer alignments
//   - configuration constraints
//
// Usage:
//   STATIC_ASSERT(sizeof(sensor_packet_t) == 12, "Packet size mismatch");
//   STATIC_ASSERT(BUFFER_SIZE % 2 == 0,          "Buffer must be even");

#define STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)

// ── Utility macros ────────────────────────────────────────────────────────────

// Silence unused variable/parameter warnings for intentionally unused items.
// Prefer fixing the warning, but use this when unavoidance (e.g. ISR params).
#define UNUSED(x) ((void)(x))

// Number of elements in a stack-allocated array.
// Do NOT use on a pointer — it will silently give the wrong answer.
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Align a value up to the nearest multiple of 'align' (must be power of 2).
// Useful for DMA buffer alignment and stack sizing.
// Example: ALIGN_UP(13, 4) == 16
#define ALIGN_UP(val, align) (((val) + ((align) - 1)) & ~((align) - 1))

// Clamp a value between a minimum and maximum.
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

// Get the minimum or maximum of two values.
// Note: evaluates arguments twice — don't use with expressions
// that have side effects like MIN(i++, j++).
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Bit manipulation
#define BIT(n)             (1UL << (n))
#define BIT_SET(reg, n)    ((reg) |= BIT(n))
#define BIT_CLEAR(reg, n)  ((reg) &= ~BIT(n))
#define BIT_TOGGLE(reg, n) ((reg) ^= BIT(n))
#define BIT_READ(reg, n)   (((reg) >> (n)) & 1UL)

// Read-modify-write a bitfield inside a register.
// Clears 'width' bits starting at 'shift' then writes 'val'.
// Example: WRITE_FIELD(GPIOA->MODER, 2, 10, 0x01)
//          writes 0b01 into bits [11:10] of MODER
#define WRITE_FIELD(reg, width, shift, val)                                                        \
    do                                                                                             \
    {                                                                                              \
        (reg) &= ~(((1UL << (width)) - 1) << (shift));                                             \
        (reg) |= (((val) & ((1UL << (width)) - 1)) << (shift));                                    \
    } while (0)

// Read a bitfield from a register.
// Example: READ_FIELD(GPIOA->MODER, 2, 10) reads bits [11:10]
#define READ_FIELD(reg, width, shift) (((reg) >> (shift)) & ((1UL << (width)) - 1))

__attribute__((always_inline)) static inline void __disable_irq(void)
{
    __asm volatile("cpsid i" ::: "memory");
}

#endif // COMMON_H
