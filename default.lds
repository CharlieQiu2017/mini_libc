PHDRS {
  headers PT_PHDR PHDRS ;
  text PT_LOAD FILEHDR PHDRS ;
  data PT_LOAD ;
}

SECTIONS {
  . = 0x400000 ;
  . = . + SIZEOF_HEADERS ;
  .text : { *(.text*) } :text
  .rodata : { *(.rodata*) } :text
  . = ALIGN(0x1000) ;
  .data : { *(.data*) } :data
  /* Mark .bss with KEEP so data segment will not be empty. Ensure VirtAddr of data != 0. */
  .bss : { KEEP(*(.bss)) *(COMMON) } :data
  /DISCARD/ : { *(.comment) *(.note.*) }
}
