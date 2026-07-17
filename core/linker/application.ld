/******************************************************************************
 * core/linker/application.ld
 * Linker script for STM32F446xE (512 KB Flash, 128 KB SRAM), Cortex-M4.
 *****************************************************************************/

ENTRY(Reset_Handler)

/* Stack/heap reserved at the top of SRAM. Adjust to your application's
 * needs; this just forces a build failure if .data + .bss + these
 * don't fit, instead of letting the stack silently grow into .bss. */
_Min_Stack_Size = 0x400;   /* 1 KB */
_Min_Heap_Size  = 0x200;   /* 512 B -- only relevant if you link malloc */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Top of SRAM = initial stack pointer value (stack grows downward) */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    /* Vector table: must be the very first thing in FLASH (address
     * 0x08000000), since the CPU fetches the initial SP and PC
     * directly from here on reset. */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* C++ exception unwinding tables -- harmless to keep in a C-only
     * project, only populated if you ever add C++ translation units */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* .data: runs from SRAM (VMA), but its initial values are stored
     * in FLASH (LMA). Reset_Handler copies LMA -> VMA before main(). */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT> FLASH

    /* .bss: zero-initialized, SRAM only, no FLASH footprint.
     * Reset_Handler zero-fills [_sbss, _ebss) before main(). */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM

    /* Reserve heap + stack space; fails the build if it doesn't fit
     * alongside .data/.bss in SRAM. */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > SRAM

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
