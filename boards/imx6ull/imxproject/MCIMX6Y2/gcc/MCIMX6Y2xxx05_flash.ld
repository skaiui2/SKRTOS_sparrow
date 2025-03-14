/*
** ###################################################################
**     Processors:          MCIMX6Y2CVM05
**                          MCIMX6Y2DVM05
**
**     Compiler:            GNU C Compiler
**     Reference manual:    IMX6ULLRM, Rev. 1, Feb. 2017
**     Version:             rev. 2.0, 2017-03-09
**     Build:               b170519
**
**     Abstract:
**         Linker file for the GNU C Compiler
**
**     Copyright 2016 Freescale Semiconductor, Inc.
**     Copyright 2016-2017 NXP
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**
**     o Neither the name of the copyright holder nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**     http:                 www.nxp.com
**     mail:                 support@nxp.com
**
** ###################################################################
*/


/* Memory region from [0x60000000-0x60001FFF] is reserved for ROM code */

/* Entry Point */
ENTRY(Reset_Handler)

HEAP_SIZE   = DEFINED(__heap_size__)           ? __heap_size__          : 0x0400;
ISTACK_SIZE = DEFINED(__irq_stack_size__)      ? __irq_stack_size__     : 0x0400;
CSTACK_SIZE = DEFINED(__stack_size__)          ? __stack_size__         : 0x0400;
RSTACK_SIZE = DEFINED(__resume_stack_size__)   ? __resume_stack_size__  : 0x0400;

/* Specify the memory areas */
MEMORY
{
  m_ocram               (RWX)  : ORIGIN = 0x00900000, LENGTH = 0x00020000
  m_interrupts          (RX)   : ORIGIN = 0x60002000, LENGTH = 0x00000040
  m_text                (RX)   : ORIGIN = 0x60002040, LENGTH = 0x003FDFC0
  m_data                (RW)   : ORIGIN = 0x80000000, LENGTH = 0x00400000
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into DDR RAM */
  .interrupts :
  {
    __VECTOR_TABLE = .;
    . = ALIGN(4);
    KEEP(*(.isr_vector))     /* Startup code */
    . = ALIGN(4);
  } > m_interrupts

  /* The program code and other data goes into DDR RAM */
  .text :
  {
    . = ALIGN(4);
    *(.text)                 /* .text sections (code) */
    *(.text*)                /* .text* sections (code) */
    *(.rodata)               /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)              /* .rodata* sections (constants, strings, etc.) */
    *(.glue_7)               /* glue arm to thumb code */
    *(.glue_7t)              /* glue thumb to arm code */
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
  } > m_text

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > m_text

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > m_text

 .ctors :
  {
    __CTOR_LIST__ = .;
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    __CTOR_END__ = .;
  } > m_text

  .dtors :
  {
    __DTOR_LIST__ = .;
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    __DTOR_END__ = .;
  } > m_text

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > m_text

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > m_text

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > m_text

  __etext = .;    /* define a global symbol at end of code */
  __NDATA_ROM = .; /* Symbol is used by startup for data initialization */

  .ncache : AT(__NDATA_ROM) ALIGN(1024*1024)
  {
    __noncachedata_start__ = .;   /* create a global symbol at ncache data start */
    *(NonCacheable)
    . = ALIGN(4);
    __noncachedata_end__ = .;     /* define a global symbol at ncache data end */
  } > m_data

  __DATA_ROM = __NDATA_ROM + SIZEOF(.ncache); /* Symbol is used by startup for data initialization */

  .data : AT(__DATA_ROM) ALIGN(1024*1024)
  {
    __DATA_RAM = .;
    __data_start__ = .;           /* create a global symbol at data start */
    *(.data)                      /* .data sections */
    *(.data*)                     /* .data* sections */
    KEEP(*(.jcr*))
    . = ALIGN(4);
    __data_end__ = .;             /* define a global symbol at data end */
  } > m_data

  /* define a global symbol at start of cacheable data */
  __ODATA_ROM = __DATA_ROM + SIZEOF(.data);

  OcramData : AT(__ODATA_ROM)
  {
    . = ALIGN(4);
    __ocram_data_start__ = .;
    *(OcramData)
    *(OcramText)
    . = ALIGN(4);
    __ocram_data_end__ = .;
  } > m_ocram

  __DATA_END = __ODATA_ROM + SIZEOF(OcramData);
  text_end = ORIGIN(m_text) + LENGTH(m_text);
  ASSERT(__DATA_END <= text_end, "region m_text overflowed with text and data")

  /* Uninitialized data section */
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    . = ALIGN(4);
    __START_BSS = .;
    __bss_start__ = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
    __END_BSS = .;
  } > m_data

  .heap :
  {
    . = ALIGN(8);
    __end__ = .;
    PROVIDE(end = .);
    __HeapBase = .;
    . += HEAP_SIZE;
    __HeapLimit = .;
    __heap_limit = .; /* Add for _sbrk */
  } > m_data

  .stack :
  {
    . = ALIGN(8);
    . += CSTACK_SIZE;
    . += ISTACK_SIZE;
  } > m_data

  /* Initializes stack on the end of block */
  __IStackTop   = ORIGIN(m_data) + LENGTH(m_data);
  __IStackLimit = __IStackTop - ISTACK_SIZE;
  __CStackTop   = __IStackLimit;
  __CStackLimit = __CStackTop - CSTACK_SIZE;
  PROVIDE(__stack = __CStackTop);

  .ARM.attributes 0 : { *(.ARM.attributes) }

  ASSERT(__CStackLimit >= __HeapLimit, "region m_data overflowed with stack and heap")

  .rstack :
  {
    . = ALIGN(8);
    . += RSTACK_SIZE;
  } > m_ocram

  /* Initializes stack on the end of block */
  __RStackTop         = ORIGIN(m_ocram) + LENGTH(m_ocram);
  __RStackLimit       = __RStackTop - RSTACK_SIZE;
  ASSERT(__RStackLimit >= __ocram_data_end__, "region m_ocram overflowed with stack and data")
}

