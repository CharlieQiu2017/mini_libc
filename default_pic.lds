PHDRS {
  headers PT_PHDR PHDRS ;
  text PT_LOAD FILEHDR PHDRS ;
  data PT_LOAD ;
  dynamic PT_DYNAMIC ;
  dropped PT_LOAD ;
}

SECTIONS {
  . = 0x10000 ;
  . = SIZEOF_HEADERS ;
  .text : { *(.text*) } :text
  .rodata : { *(.rodata*) } :text
  . = ALIGN(0x1000) ;
  .data : { *(.data*) } :data
  .bss : { *(.bss) } :data
  . = ALIGN(0x1000) ;
  .dynamic : { *(.dynamic) } :dropped :dynamic
  .got : { *(.got) } :dropped
  .got.plt : { *(.got.plt) } :dropped
  .dynsym : { *(.dynsym) } :dropped
  .dynstr : { *(.dynstr) } :dropped
  .hash : { *(.hash) } :dropped
}
