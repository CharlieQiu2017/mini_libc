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
  .bss : { KEEP(*(.bss)) *(COMMON) } :data
  .got : { *(.got) } : data
  .got.plt : { *(.got.plt) } : data
  /DISCARD/ : { *(.dynamic) *(.dynsym) *(.dynstr) *(.hash) }
}
