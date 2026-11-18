/* Linker script for WCH CH32V203F8U6 (64 KB flash, 20 KB SRAM).
 *
 * Memory map (CH32V203 datasheet V3.0):
 *   FLASH R0WAIT   0x00000000   64 KB   (zero-wait execution area)
 *   FLASH non-R0W  0x08000000  160 KB   (data area of the 224 KB code flash)
 *   SRAM           0x20000000   20 KB   (F8x6: 20 KB of the max 64 KB)
 *   Bootloader     0x1FFF F880   28 KB   (system, aliased — not ours)
 *
 * We link only into the zero-wait area; the non-zero-wait area is available
 * for large constant data if a future version needs it.
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size  = 0x200;   /* 512 B reserved for the heap */
_Min_Stack_Size = 0x400;   /* 1 KB reserved for the stack */

SECTIONS
{
    .init :
    {
        _sinit = .;
        KEEP(*(SORT_NONE(.init)))
        . = ALIGN(4);
        KEEP(*(SORT_NONE(.init.*)))
        . = ALIGN(4);
        _einit = .;
    } >FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        . = ALIGN(4);
    } >FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } >FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT>FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    .heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /DISCARD/ :
    {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame)
    }
}
