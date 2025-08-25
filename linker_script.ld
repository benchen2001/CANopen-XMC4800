/* XMC4800 連結腳本 - 簡化版本 */

/* 記憶體佈局 */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K  /* 2MB Flash */
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 352K   /* 352KB RAM */
}

/* 堆疊大小 */
_stack_size = 0x2000;  /* 8KB 堆疊 */

/* 程式進入點 */
ENTRY(Reset_Handler)

/* 段定義 */
SECTIONS
{
    /* 中斷向量表 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* 程式碼段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* 資料段 (初始化的全域變數) */
    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    /* BSS 段 (未初始化的全域變數) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* 堆疊 */
    .stack :
    {
        . = ALIGN(4);
        _sstack = .;
        . = . + _stack_size;
        . = ALIGN(4);
        _estack = .;
    } > RAM

    /* 移除除錯段以減少檔案大小 */
    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
        *(.debug*)
    }
}

/* 提供符號給 C 程式使用 */
_sidata = LOADADDR(.data);