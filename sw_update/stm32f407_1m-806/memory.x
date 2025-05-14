MEMORY
{
  FLASH : ORIGIN = 0x08000000, LENGTH = 384k
  UPPER_FLASH : ORIGIN = 0x08061000, LENGTH = 380k
  RAM : ORIGIN = 0x20000000, LENGTH = 112K
}

/* This is where the call stack will be allocated. */
/* The stack is of the full descending type. */
/* NOTE Do NOT modify `_stack_start` unless you know what you are doing */
_stack_start = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .text :
  {
    *(.text .text.*);
  } > UPPER_FLASH

  .rodata :
  {
    *(..rodata .rodata.*);
  } > UPPER_FLASH

}