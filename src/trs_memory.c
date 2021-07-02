/*
 * Copyright (C) 1992 Clarendon Hill Software.
 *
 * Permission is granted to any individual or institution to use, copy,
 * or redistribute this software, provided this copyright notice is retained.
 *
 * This software is provided "as is" without any expressed or implied
 * warranty.  If this software brings on any sort of damage -- physical,
 * monetary, emotional, or brain -- too bad.  You've got no one to blame
 * but yourself.
 *
 * The software may be modified for your own purposes, but modified versions
 * must retain this notice.
 */

/*
 * Copyright (c) 1996-2020, Timothy P. Mann
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * trs_memory.c -- memory emulation functions for the TRS-80 emulator
 *
 * Routines in this file perform operations such as mem_read and mem_write,
 * and are the top level emulation points for memory-mapped devices such
 * as the screen and keyboard.
 */

#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "trs.h"
#include "trs_disk.h"
#include "trs_hard.h"
#include "trs_imp_exp.h"
#include "trs_state_save.h"

#define MAX_ROM_SIZE	(0x3800)
#define MAX_VIDEO_SIZE	(0x0800)
#define CP500_ROM_SIZE	(0x4000)

#define MAX_MEMORY_SIZE	(0x200000)

/* 512K is the largest we support. There were it seems 1MByte
   options at some point which is the full range of the mapping.
   How the mapping register worked for > 1MB is not known */
#define MAX_SUPERMEM_SIZE 	(0x80000)

/* Locations for Model I, Model III, and Model 4 map 0 */
#define KEYBOARD_START	(0x3800)
#define VIDEO_START	(0x3c00)
#define RAM_START	(0x4000)
#define PRINTER_ADDRESS	(0x37E8)

/* Interrupt latch register in EI (Model 1) */
#define TRS_INTLATCH(addr) (((addr)&~3) == 0x37e0)

/* We allow for 2MB of banked memory via port 0x94. That is the extreme limit
   of the port mods rather than anything normal (512K might be more 'normal' */
Uint8 memory[MAX_MEMORY_SIZE + 1]; /* +1 so strings from mem_pointer are NUL-terminated */
Uint8 rom[MAX_ROM_SIZE + 1];
Uint8 cp500_rom[CP500_ROM_SIZE + 1];
int trs_rom_size;
int lowercase = 1;
int romin; /* Model 4p */
int huffman_ram;
int hypermem;
int supermem;
int selector;
int eg3200; /* EACA EG 3200 Genie III */

/* private data */
static Uint8 video[MAX_VIDEO_SIZE + 1];
/* We map the SuperMem separately, otherwise it can get really
   confusing when combining with other stuff */
static Uint8 supermem_ram[MAX_SUPERMEM_SIZE + 1];
static int trs_video_size;
static int memory_map;
static int bank_offset[2];
#define VIDEO_PAGE_0 0
#define VIDEO_PAGE_1 1024
static int video_offset = (-VIDEO_START + VIDEO_PAGE_0);
static unsigned int bank_base = 0x10000;
static Uint8 mem_command;
static int supermem_base;
static unsigned int supermem_hi;
static int selector_reg;
static int m_a11_flipflop;
static int system_byte;

void mem_video_page(int which)
{
    video_offset = -VIDEO_START + (which ? VIDEO_PAGE_1 : VIDEO_PAGE_0);
}

void mem_bank(int command)
{
    switch (command) {
      case 0:
        /* L64 Lower / Upper */
	bank_offset[0] = 0 << 15;
	bank_offset[1] = 0 << 15;
	break;
      case 2:
        /* L64 Lower / H64 Lower */
	bank_offset[0] = 0 << 15;
	bank_offset[1] = bank_base - (1 << 15);
	break;
      case 3:
        /* L64 Lower / H64 upper */
	bank_offset[0] = 0 << 15;
	bank_offset[1] = (0 << 15) + bank_base;
	break;
      case 6:
        /* H64 Lower / L64 upper */
	bank_offset[0] = (0 << 15) + bank_base;
	bank_offset[1] = 0 << 15;
	break;
      case 7:
        /* H64 Upper / L64 Upper */
	bank_offset[0] = (1 << 15) + bank_base;
	bank_offset[1] = 0 << 15;
	break;
      default:
	error("unknown mem_bank command %d", command);
	break;
    }
    mem_command = command;
}

/*
 *	Dave Huffman (and some other) memory expansions. These decode
 *	port 0x94 off U50 as follows
 *
 *	7: only used with Z180 board (not emulated - would need Z180 emulation!)
 *	6: write protect - not emulated
 *	5: sometimes used for > 4MHz turbo mod
 *	4-0: Bits A20-A16 of the alt bank
 *
 *	Set to 1 on a reset so that you get the 'classic' memory map
 *	This port is read-write and the drivers depend upon it
 *	(See RAMDV364.ASM)
 *
 *	The Hypermem is very similar and also changes the upper
 *	64K bank between multiple banks. However the values
 *	are on port 0x90 (sound) bits 4-1, which is a much poorer
 *	design IMHO as sound using apps can randomly change the
 *	upper bank. Fine for a ramdisc but means other software
 *	must take great care.
 *
 *	The MegaMem mappings are not known and not emulated.
 */

void mem_bank_base(int bits)
{
	if (huffman_ram) {
		bits &= 0x1F;
		bank_base = bits << 16;
		mem_bank(mem_command);
	}
	if (hypermem) {
	        /* HyperMem replaces the upper 64K bank with multiple
	           banks according to port 0x90 bits 4-1 */
		bits &= 0x1E;
		/* 0 base is upper bank of 64K */
		bits += 2;
		bank_base = bits << 15;
		mem_bank(mem_command);
	}
	if (supermem) {
		/* Emulate a 512Kb system. A standard model 1 SuperMEM is 256K
		   or 512K with double stacked chips */
		bits &= 0x0F; /* 15 bits of address + 4bits logical */
		supermem_base = bits << 15;
		/* The supermem can flip the low or high 32K. Set
		   bit 5 to map low */
		if (bits & 0x20)
		    supermem_hi = 0x0000;
		else
		    supermem_hi = 0x8000;
	}
}

int mem_read_bank_base(void)
{
	if (huffman_ram)
		return (bank_base >> 16) & 0x1F;
	if (supermem)
		return (supermem_base >> 15) |
			((supermem_hi == 0) ? 0x20 : 0);
	/* And the HyperMem appears to be write-only */
	return 0xFF;
}

/* The A11 flipflop is used for enabling access to the CP-500
   system monitor code at the EPROM address range 3800-3fff */
int cp500_a11_flipflop_toggle(void)
{
	/* toggle the flip-flop at every read at io addresses 0xf4-f7 */
	m_a11_flipflop ^= 1;

	memcpy(&rom[0], &cp500_rom[m_a11_flipflop * 0x800], MAX_ROM_SIZE);

	return 0x00; /* really?! */
}

void eg3200_bank_out(Uint8 value)
{
	if (eg3200 == 0) {
		eg3200 = value;
		trs_disk_doubler = TRSDISK_PERCOM;
		trs_timer_init();
		trs_screen_inverse(0);
		trs_screen_init();
	} else {
		eg3200 = value;
	}
}

void selector_out(Uint8 value)
{
	/* Not all bits are necessarily really present but hey what
	   you can't read back you can't tell */
	selector_reg = value;
	/* Always Model 1 */
	memory_map = (trs_model << 4) + (selector_reg & 7);
	/* 0x10 is already the default tandy map we add 11-17 in the style
	   of the model 4 approach */
	if (selector_reg & 0x8) {
		/* External RAM enabled */
		/* Effectively the selector bits << 15 */
		/* Low 64K is the base memory */
		bank_base = 32768 + ((selector_reg & 0x30) << 11);
		/* Now are we mapping it high or low */
		if (selector_reg & 1) /* Low */
			bank_base += 32768;
	} else
		bank_base = 0;
}

void sys_byte_out(Uint8 value)
{
	/* TRS-80 Model I memory map */
	memory_map = 0x10;

	if (speedup > 4) {
		/* TCS SpeedMaster CP/M banking */
		if (speedup == 6) {
			if (value & 1)
				memory_map = 0x14;
		}
		/* HRG only in TRS-80 memory map */
		if (memory_map == 0x10) {
			if ((value & (1 << 3)))
				memory_map = 0x20;
			hrg_onoff((value & (1 << 1)));
		}
	} else {
		/* Other CP/M banking */
		switch (value) {
			case 0x10:
			case 0x11:
			case 0x54:
				memory_map = 0x14;
				break;
		}
	}
	system_byte = value;
}

Uint8 sys_byte_in(void)
{
	return system_byte;
}

static void mem_init(void)
{
    /* Initialize RAM, ROM & Video memory */
    memset(&memory, 0, sizeof(memory));
    memset(&supermem_ram, 0, sizeof(supermem_ram));
    memset(&rom, 0, sizeof(rom));
    memset(&cp500_rom, 0, sizeof(cp500_rom));
    memset(&video, 0, sizeof(video));

    if (trs_model < 4) {
        trs_video_size = 1024;
        /* Fill memory of random seed buffer */
        memset(&memory[0x4090], 0x7F, 0x3);
    }
    else
        trs_video_size = MAX_VIDEO_SIZE;

    mem_map(0);
    mem_bank(0);
    mem_video_page(0);
}

/* Handle reset button if poweron=0;
   handle hard reset or initial poweron if poweron=1 */
void trs_reset(int poweron)
{
    trs_emu_mouse = FALSE;
    m_a11_flipflop = 0;
    bank_base = 0x10000;
    eg3200 = 0;
    mem_command = 0;
    supermem_base = 0;
    supermem_hi = 0x8000;

    /* Close disks opened by Z80 programs */
    do_emt_resetdisk();
    /* Reset devices (Model I SYSRES, Model III/4 RESET) */
    trs_cassette_reset();
    trs_disk_init(poweron); /* also inits trs_hard and trs_stringy */

    if (trs_model == 5) {
        /* Switch in boot ROM */
	z80_out(0x9C, 1);
    }
    if (trs_model >= 4) {
        /* Turn off various memory map and video mode bits */
	z80_out(0x84, 0);
	if (huffman_ram)
		z80_out(0x94, 0);
        if (hypermem)
                z80_out(0x90, 0);
    }
    if (trs_model >= 3) {
	grafyx_write_mode(0);
	trs_interrupt_mask_write(0);
	trs_nmi_mask_write(0);
    }
    if (trs_model == 3) {
        grafyx_m3_reset();
    }
    if (trs_model == 1) {
	hrg_onoff(0);		/* Switch off HRG1B hi-res graphics. */
	bank_base = 0;
	selector_reg = 0;
	system_byte = 0;
    }
    trs_kb_reset();  /* Part of keyboard stretch kludge */
    clear_key_queue(); /* init the key queue */

    trs_cancel_event();
    trs_timer_interrupt(0);
    if (poweron || trs_model >= 4) {
        /* Reset processor */
	z80_reset();
	mem_init();
	trs_rom_init();
	trs_timer_init();
	if (trs_show_led) {
	  trs_disk_led(-1, -1);
	  trs_hard_led(-1, -1);
	  trs_turbo_led();
	}
    } else {
	trs_timer_speed(0);
	/* Signal a nonmaskable interrupt. */
	trs_reset_button_interrupt(1);
	trs_schedule_event(trs_reset_button_interrupt, 0, 2000);
    }
    /* Clear screen */
    screen_init();
}

void mem_map(int which)
{
    memory_map = which + (trs_model << 4) + (romin << 2);
}

void mem_romin(int state)
{
    romin = (state & 1);
    memory_map = (memory_map & ~4) + (romin << 2);
}

/*
 * hack to let us initialize the ROM memory
 */
void mem_write_rom(unsigned int address, int value)
{
    if (address < MAX_ROM_SIZE)
      rom[address] = value;

    if (address < CP500_ROM_SIZE)
      cp500_rom[address] = value;
}

static int trs80_model1_ram(int address)
{
  int bank = 0x8000;
  int offset = address;

  /* Model 4 doesn't have banking in Model 1 mode */
  if (trs_model != 1)
    return memory[address];
  /* Selector mode 6 remaps RAM from 0000-3FFF to C000-FFFF while keeping
     the ROMs visible */
  /* selector_reg is always 0 if selector disabled so we don't need an
     extra check */
  if ((selector_reg & 7) == 6) {
    if (address >= 0xC000) {
      /* Use the low 16K, and then bank it. I'm not 100% sure how the
         PAL orders the two */
      offset &= 0x3FFF;
    }
  }
  /* Bank low on odd modes */
  if ((selector_reg & 1) == 1)
    bank = 0;
  /* Deal with 32K banking from selector */
  if ((address & 0x8000) == bank)
    offset += bank_base;
  return memory[offset];
}

static int trs80_model1_mmio(int address)
{
  if (address >= VIDEO_START) return video[address + video_offset];
  if (address < trs_rom_size) return rom[address];
  if (address == TRSDISK_DATA) return trs_disk_data_read();
  if (TRS_INTLATCH(address)) return trs_interrupt_latch_read();
  if (address == TRSDISK_STATUS) return trs_disk_status_read();
  if (address == PRINTER_ADDRESS) return trs_printer_read();
  if (address == TRSDISK_TRACK) return trs_disk_track_read();
  if (address == TRSDISK_SECTOR) return trs_disk_sector_read();
  /* With a selector 768 bytes poke through the hole */
  if (address >= 0x3900 && selector)
    return trs80_model1_ram(address);
  if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
  return 0xff;
}

int mem_read(int address)
{
    address &= 0xffff; /* allow callers to be sloppy */

    /* There are some adapters that sit above the system and
       either intercept before the hardware proper, or adjust
       the address. Deal with these first so that we take their
       output and feed it into the memory map */

    /* EG 3200 bank switching, inverted (bit set to 0 => bank enabled) */
    if (eg3200) {
      /* Bit 0 - Bank 1: ROM/EPROM */
      if ((eg3200 & (1 << 0)) == 0) {
	if (address < trs_rom_size)
	  return rom[address];
      }
      /* Bit 1 - Bank 2: Video Memory 0 (1k, 64x16, TRS-80 M1 compatible) */
      if ((eg3200 & (1 << 1)) == 0) {
	if (address >= VIDEO_START && address <= 0x3FFF)
	  return video[address - VIDEO_START];
      }
      /* Bit 2 - Bank 3: Video Memory 1 (additional 1k for 80x24 video mode) */
      if ((eg3200 & (1 << 2)) == 0) {
	if (address >= 0x4000 && address <= 0x43FF)
	  return video[address - VIDEO_START];
      }
      /* Bit 3 - Bank 4: Disk I/O and Keyboard */
      if ((eg3200 & (1 << 3)) == 0) {
	if (address >= 0x37E0 && address <= 0x37EF)
	  return trs80_model1_mmio(address);
	if (address >= KEYBOARD_START) {
	  if (address <= 0x38C0) return trs_kb_mem_read(address);
	  if (address <= 0x38FF) return 0x00;
	}
      }
      /* Bank 0: RAM */
      return memory[address];
    }
    /* The SuperMem sits between the system and the Z80 */
    if (supermem) {
      if (!((address ^ supermem_hi) & 0x8000))
          return supermem_ram[supermem_base + (address & 0x7FFF)];
      /* Otherwise the request comes from the system */
    }
    switch (memory_map) {
      case 0x10: /* Model I */
        if (address < RAM_START)
	  return trs80_model1_mmio(address & 0x3FFF);
	else
	  return trs80_model1_ram(address);
      case 0x11: /* Model 1: selector mode 1 (all RAM except I/O high */
        if (address >= 0xF7E0 && address <= 0xF7FF)
          return trs80_model1_mmio(address & 0x3FFF);
	return trs80_model1_ram(address);
      case 0x12: /* Model 1 selector mode 2 (ROM disabled) */
        if (address < 0x37E0)
          return trs80_model1_ram(address);
	if (address < RAM_START)
	  return trs80_model1_mmio(address);
	return trs80_model1_ram(address);
      case 0x13: /* Model 1: selector mode 3 (CP/M mode) */
        if (address >= 0xF7E0)
          return trs80_model1_mmio(address & 0x3FFF);
	/* Fall through */
      case 0x14: /* Model 1: All RAM banking high */
      case 0x15: /* Model 1: All RAM banking low */
	return trs80_model1_ram(address);
      case 0x16: /* Model 1: Low 16K in top 16K */
	if (address < RAM_START)
	  return trs80_model1_mmio(address);
	return trs80_model1_ram(address);
      case 0x17: /* Model 1: Described in the selector doc as 'not useful' */
        return 0xFF;	/* Not clear what really happens */
      case 0x20: /* LNW80/TCS SpeedMaster: HRG in low 16K */
	if (address < RAM_START)
	  return hrg_read_data();
	return trs80_model1_ram(address);

      case 0x30: /* Model III */
	if (address >= RAM_START) return memory[address];
	if (address == PRINTER_ADDRESS)	return trs_printer_read();
	if (address < trs_rom_size) return rom[address];
	if (address >= VIDEO_START) {
	  return grafyx_m3_read_byte(address - VIDEO_START);
	}
	if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
	return 0xff;

      case 0x40: /* Model 4 map 0 */
	if (address >= RAM_START) {
	    return memory[address + bank_offset[address >> 15]];
	}
	if (address == PRINTER_ADDRESS) return trs_printer_read();
	if (address < trs_rom_size) return rom[address];
	if (address >= VIDEO_START) {
	    return video[address + video_offset];
	}
	if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
	return 0xff;

      case 0x54: /* Model 4P map 0, boot ROM in */
      case 0x55: /* Model 4P map 1, boot ROM in */
	if (address < trs_rom_size) return rom[address];
	/* else fall thru */
      case 0x41: /* Model 4 map 1 */
      case 0x50: /* Model 4P map 0, boot ROM out */
      case 0x51: /* Model 4P map 1, boot ROM out */
	if (address >= RAM_START || address < KEYBOARD_START) {
	    return memory[address + bank_offset[address >> 15]];
	}
	if (address >= VIDEO_START) {
	    return video[address + video_offset];
	}
	if (address >= KEYBOARD_START) return trs_kb_mem_read(address);
	return 0xff;

      case 0x42: /* Model 4 map 2 */
      case 0x52: /* Model 4P map 2, boot ROM out */
      case 0x56: /* Model 4P map 2, boot ROM in */
	if (address < 0xf400) {
	    return memory[address + bank_offset[address >> 15]];
	}
	if (address >= 0xf800) return video[address-0xf800];
	return trs_kb_mem_read(address);

      case 0x43: /* Model 4 map 3 */
      case 0x53: /* Model 4P map 3, boot ROM out */
      case 0x57: /* Model 4P map 3, boot ROM in */
	return memory[address + bank_offset[address >> 15]];

    }
    /* not reached */
    return 0xff;
}

static void trs80_screen_write_char(int vaddr, int value)
{
  vaddr &= 0x7ff;

  if (video[vaddr] != value) {
      video[vaddr] = value;
      trs_screen_write_char(vaddr, value);
  }
}

static void trs80_model1_write_mem(int address, int value)
{
  int bank = 0x8000;
  int offset = address;

  /* Model 4 doesn't have banking in Model 1 mode */
  if (trs_model != 1) {
    memory[address] = value;
    return;
  }
  /* Selector mode 6 remaps RAM from 0000-3FFF to C000-FFFF while keeping
     the ROMs visible */
  /* selector_reg is always 0 if selector disabled so we don't need an
     extra check */
  if ((selector_reg & 7) == 6) {
    if (address >= 0xC000) {
      /* We have no low 16K of RAM. This is for the LNW80 really */
      if (!(selector_reg & 8))
	return;
      /* Use the low 16K, and then bank it. I'm not 100% sure how the
         PAL orders the two */
      offset &= 0x3FFF;
    }
  }
  /* Bank low on odd modes */
  if ((selector_reg & 1) == 1)
    bank = 0;
  /* Deal with 32K banking from selector or supermem */
  if ((address & 0x8000) == bank)
    offset += bank_base;
  memory[offset] = value;
}

static void trs80_model1_write_mmio(int address, int value)
{
  if (address >= VIDEO_START) {
    if (!lowercase) {
      /*
       * Video write.  Hack here to make up for the missing bit 6
       * video ram, emulating the gate in Z30.
       */
      if (trs_model == 1) {
        if (value & 0xa0)
          value &= 0xbf;
        else
          value |= 0x40;
      }
    }
    trs80_screen_write_char(address + video_offset, value);
  } else if (address == PRINTER_ADDRESS) {
    trs_printer_write(value);
  } else if (address == TRSDISK_DATA) {
    trs_disk_data_write(value);
  } else if (address == TRSDISK_COMMAND) {
    trs_disk_command_write(value);
  } else if (address == TRSDISK_TRACK) {
    trs_disk_track_write(value);
  } else if (address == TRSDISK_SECTOR) {
    trs_disk_sector_write(value);
  } else if (TRSDISK_SELECT(address)) {
    trs_disk_select_write(value);
  } else if (address >= 0x3900)
    trs80_model1_write_mem(address, value);
}

void mem_write(int address, int value)
{
    address &= 0xffff;

    /* EG 3200 bank switching, inverted (bit set to 0 => bank enabled) */
    if (eg3200) {
      /* Bit 1 - Bank 2: Video Memory 0 (1k, 64x16, TRS-80 M1 compatible) */
      if ((eg3200 & (1 << 1)) == 0) {
	if (address >= VIDEO_START && address <= 0x3FFF) {
	  trs80_screen_write_char(address - VIDEO_START, value);
	  return;
	}
      }
      /* Bit 2 - Bank 3: Video Memory 1 (additional 1k for 80x24 video mode) */
      if ((eg3200 & (1 << 2)) == 0) {
	if (address >= 0x4000 && address <= 0x43FF) {
	  trs80_screen_write_char(address - VIDEO_START, value);
	  return;
	}
      }
      /* Bit 3 - Bank 4: Disk I/O */
      if ((eg3200 & (1 << 3)) == 0) {
	if (address >= 0x37E0 && address <= 0x37EF) {
	  trs80_model1_write_mmio(address, value);
	  return;
	}
      }
      /* Bank 0: RAM */
      memory[address] = value;
      return;
    }
    /* The SuperMem sits between the system and the Z80 */
    if (supermem) {
      if (!((address ^ supermem_hi) & 0x8000)) {
          supermem_ram[supermem_base + (address & 0x7FFF)] = value;
          return;
      }
      /* Otherwise the request comes from the system */
    }

    switch (memory_map) {
      case 0x10: /* Model I */
        if (address >= RAM_START)
          trs80_model1_write_mem(address, value);
	else
	  trs80_model1_write_mmio(address, value);
	break;
      case 0x11: /* Model 1: selector mode 1 (all RAM except I/O high */
        if (address >= 0xF7E0 && address <= 0xF7FF)
          trs80_model1_write_mmio(address & 0x3FFF, value);
	else
	  trs80_model1_write_mem(address, value);
	break;
      case 0x12: /* Model 1 selector mode 2 (ROM disabled) */
        if (address < 0x37E0)
          trs80_model1_write_mem(address, value);
	else if (address < RAM_START)
	  trs80_model1_write_mmio(address, value);
	else
	  trs80_model1_write_mem(address, value);
	break;
      case 0x13: /* Model 1: selector mode 3 (CP/M mode) */
        if (address >= 0xF7E0)
          trs80_model1_write_mmio(address & 0x3FFF, value);
	/* Fall through */
      case 0x14: /* Model 1: All RAM banking high */
      case 0x15: /* Model 1: All RAM banking low */
	trs80_model1_write_mem(address, value);
	break;
      case 0x16: /* Model 1: Low 16K in top 16K */
	if (address < RAM_START)
	  trs80_model1_write_mmio(address, value);
	else
	  trs80_model1_ram(address);
      case 0x17: /* Model 1: Described in the selector doc as 'not useful' */
        break;	/* Not clear what really happens */
      case 0x20: /* LNW80/TCS SpeedMaster: HRG in low 16K */
	if (address < RAM_START) {
	  hrg_write_addr(address, 0x3FFF);
	  hrg_write_data(value);
	} else {
	  trs80_model1_write_mem(address, value);
	}
	break;

      case 0x30: /* Model III */
	if (address >= RAM_START) {
	    memory[address] = value;
	} else if (address >= VIDEO_START) {
	    if (grafyx_m3_write_byte(address + video_offset, value)) return;
	    trs80_screen_write_char(address + video_offset, value);
	} else if (address == PRINTER_ADDRESS) {
	    trs_printer_write(value);
	}
	break;

      case 0x40: /* Model 4 map 0 */
      case 0x50: /* Model 4P map 0, boot ROM out */
      case 0x54: /* Model 4P map 0, boot ROM in */
	if (address >= RAM_START) {
	    memory[address + bank_offset[address >> 15]] = value;
	} else if (address >= VIDEO_START) {
	    trs80_screen_write_char(address + video_offset, value);
	} else if (address == PRINTER_ADDRESS) {
	    trs_printer_write(value);
	}
	break;

      case 0x41: /* Model 4 map 1 */
      case 0x51: /* Model 4P map 1, boot ROM out */
      case 0x55: /* Model 4P map 1, boot ROM in */
	if (address >= RAM_START || address < KEYBOARD_START) {
	    memory[address + bank_offset[address >> 15]] = value;
	} else if (address >= VIDEO_START) {
	    trs80_screen_write_char(address + video_offset, value);
	}
	break;

      case 0x42: /* Model 4 map 2 */
      case 0x52: /* Model 4P map 2, boot ROM out */
      case 0x56: /* Model 4P map 2, boot ROM in */
	if (address < 0xf400) {
	    memory[address + bank_offset[address >> 15]] = value;
	} else if (address >= 0xf800) {
	    trs80_screen_write_char(address - 0xf800, value);
	}
	break;

      case 0x43: /* Model 4 map 3 */
      case 0x53: /* Model 4P map 3, boot ROM out */
      case 0x57: /* Model 4P map 3, boot ROM in */
	memory[address + bank_offset[address >> 15]] = value;
	break;
    }
}

/*
 * Words are stored with the low-order byte in the lower address.
 */
int mem_read_word(int address)
{
    int rval;

    rval = mem_read(address++);
    rval |= mem_read(address & 0xffff) << 8;
    return rval;
}

void mem_write_word(int address, int value)
{
    mem_write(address++, value & 0xff);
    mem_write(address, value >> 8);
}

static Uint8 *trs80_model1_ram_addr(int address)
{
  int bank = 0x8000;
  int offset = address;

  /* Model 4 doesn't have banking in Model 1 mode */
  if (trs_model != 1)
    return memory + address;
  /* Selector mode 6 remaps RAM from 0000-3FFF to C000-FFFF while keeping
     the ROMs visible */
  /* selector_reg is always 0 if selector disabled so we don't need an
     extra check */
  if ((selector_reg & 7) == 6) {
    if (address >= 0xC000) {
      /* Use the low 16K, and then bank it. I'm not 100% sure how the
         PAL orders the two */
      offset &= 0x3FFF;
    }
  }
  /* Bank low on odd modes */
  if ((selector_reg & 1) == 1)
    bank = 0;
  /* Deal with 32K banking from selector or supermem */
  if ((address & 0x8000) == bank)
    offset += bank_base;
  return memory + offset;
}

static Uint8 *trs80_model1_mmio_addr(int address, int writing)
{
  if (address >= VIDEO_START) return video + address;
  if (address < trs_rom_size && !writing) return memory + address;
  /* With a selector 768 bytes poke through the hole */
  if (address >= 0x3900 && selector)
    return trs80_model1_ram_addr(address);
  return NULL;
}

/*
 * Get a pointer to the given address.  Note that there is no checking
 * whether the next virtual address is physically contiguous.  The
 * caller is responsible for making sure his strings don't span
 * memory map boundaries.
 *
 * Needs to die...
 */
Uint8 *mem_pointer(int address, int writing)
{
    address &= 0xffff;

    /* EG 3200 bank switching, inverted (bit set to 0 => bank enabled) */
    if (eg3200) {
      /* Bit 0 - Bank 1: ROM/EPROM */
      if ((eg3200 & (1 << 0)) == 0) {
	if (address < trs_rom_size)
	  return &rom[address];
      }
      /* Bit 1 - Bank 2: Video Memory 0 (1k, 64x16, TRS-80 M1 compatible) */
      if ((eg3200 & (1 << 1)) == 0) {
	if (address >= VIDEO_START && address <= 0x3FFF)
	  return &video[address - VIDEO_START];
      }
      /* Bit 2 - Bank 3: Video Memory 1 (additional 1k for 80x24 video mode) */
      if ((eg3200 & (1 << 2)) == 0) {
	if (address >= 0x4000 && address <= 0x43FF)
	  return &video[address - VIDEO_START];
      }
      /* Bank 0: RAM */
      return &memory[address];
    }
    /* The SuperMem sits between the system and the Z80 */
    if (supermem) {
      if (!((address ^ supermem_hi) & 0x8000))
          return &supermem_ram[supermem_base + (address & 0x7FFF)];
      /* Otherwise the request comes from the system */
    }

    switch (memory_map + (writing << 3)) {
      case 0x10: /* Model I */
      case 0x18:
        if (address < RAM_START)
	  return trs80_model1_mmio_addr(address & 0x3FFF, writing);
	else
	  return trs80_model1_ram_addr(address);
      case 0x11: /* Model 1: selector mode 1 (all RAM except I/O high */
      case 0x19:
	return trs80_model1_ram_addr(address);
      case 0x12: /* Model 1 selector mode 2 (ROM disabled) */
        if (address < 0x37E0)
          return trs80_model1_ram_addr(address);
	if (address < RAM_START)
	  return trs80_model1_mmio_addr(address, writing);
	return trs80_model1_ram_addr(address);
      case 0x13: /* Model 1: selector mode 3 (CP/M mode) */
        if (address >= 0xF7E0)
          return trs80_model1_mmio_addr(address & 0x3FFF, writing);
	/* Fall through */
      case 0x14: /* Model 1: All RAM banking high */
      case 0x15: /* Model 1: All RAM banking low */
	return trs80_model1_ram_addr(address);
      case 0x16: /* Model 1: Low 16K in top 16K */
	if (address < RAM_START)
	  return trs80_model1_mmio_addr(address, writing);
	return trs80_model1_ram_addr(address);
      case 0x17: /* Model 1: Described in the selector doc as 'not useful' */
	return NULL;	/* Not clear what really happens */
      case 0x30: /* Model III reading */
        if (trs_model < 4 && address >= 32768)
	    return &memory[address + bank_base];
	if (address >= VIDEO_START) return &memory[address];
	if (address < trs_rom_size) return &rom[address];
	return NULL;

      case 0x38: /* Model III writing */
	if (address >= RAM_START) return &memory[address];
	if (address >= VIDEO_START) return &video[address + video_offset];
	return NULL;

      case 0x40: /* Model 4 map 0 reading */
	if (address >= RAM_START) {
	    return &memory[address + bank_offset[address >> 15]];
	}
	if (address < trs_rom_size) return &rom[address];
	if (address >= VIDEO_START) {
	    return &video[address + video_offset];
	}
	return NULL;

      case 0x48: /* Model 4 map 0 writing */
      case 0x58: /* Model 4P map 0, boot ROM out, writing */
      case 0x5c: /* Model 4P map 0, boot ROM in, writing */
	if (address >= RAM_START) {
	    return &memory[address + bank_offset[address >> 15]];
	}
	if (address >= VIDEO_START) {
	    return &video[address + video_offset];
	}
	return NULL;

      case 0x54: /* Model 4P map 0, boot ROM in, reading */
      case 0x55: /* Model 4P map 1, boot ROM in, reading */
	if (address < trs_rom_size) return &rom[address];
	/* else fall thru */
      case 0x41: /* Model 4 map 1 reading */
      case 0x49: /* Model 4 map 1 writing */
      case 0x50: /* Model 4P map 0, boot ROM out, reading */
      case 0x51: /* Model 4P map 1, boot ROM out, reading */
      case 0x59: /* Model 4P map 1, boot ROM out, writing */
      case 0x5d: /* Model 4P map 1, boot ROM in, writing */
	if (address >= RAM_START || address < KEYBOARD_START) {
	    return &memory[address + bank_offset[address >> 15]];
	}
	if (address >= VIDEO_START) {
	    return &video[address + video_offset];
	}
	return NULL;

      case 0x42: /* Model 4 map 1, reading */
      case 0x4a: /* Model 4 map 1, writing */
      case 0x52: /* Model 4P map 2, boot ROM out, reading */
      case 0x5a: /* Model 4P map 2, boot ROM out, writing */
      case 0x56: /* Model 4P map 2, boot ROM in, reading */
      case 0x5e: /* Model 4P map 2, boot ROM in, writing */
	if (address < 0xf400) {
	    return &memory[address + bank_offset[address >> 15]];
	}
	if (address >= 0xf800) return &video[address-0xf800];
	return NULL;

      case 0x43: /* Model 4 map 3, reading */
      case 0x4b: /* Model 4 map 3, writing */
      case 0x53: /* Model 4P map 3, boot ROM out, reading */
      case 0x5b: /* Model 4P map 3, boot ROM out, writing */
      case 0x57: /* Model 4P map 3, boot ROM in, reading */
      case 0x5f: /* Model 4P map 3, boot ROM in, writing */
	return &memory[address + bank_offset[address >> 15]];
    }
    /* not reached */
    return NULL;
}

void trs_mem_save(FILE *file)
{
  trs_save_uint8(file, memory, MAX_MEMORY_SIZE + 1);
  trs_save_uint8(file, supermem_ram, MAX_SUPERMEM_SIZE + 1);
  trs_save_uint8(file, rom, MAX_ROM_SIZE + 1);
  trs_save_uint8(file, cp500_rom, CP500_ROM_SIZE + 1);
  trs_save_uint8(file, video, MAX_VIDEO_SIZE + 1);
  trs_save_int(file, &trs_rom_size, 1);
  trs_save_int(file, &trs_video_size, 1);
  trs_save_int(file, &memory_map, 1);
  trs_save_int(file, bank_offset, 2);
  trs_save_int(file, &video_offset, 1);
  trs_save_int(file, &romin, 1);
  trs_save_uint32(file, &bank_base, 1);
  trs_save_uint8(file, &mem_command, 1);
  trs_save_int(file, &huffman_ram, 1);
  trs_save_int(file, &hypermem, 1);
  trs_save_int(file, &supermem, 1);
  trs_save_int(file, &selector, 1);
  trs_save_int(file, &selector_reg, 1);
  trs_save_int(file, &m_a11_flipflop, 1);
  trs_save_int(file, &eg3200, 1);
  trs_save_int(file, &system_byte, 1);
}

void trs_mem_load(FILE *file)
{
  trs_load_uint8(file, memory, MAX_MEMORY_SIZE + 1);
  trs_load_uint8(file, supermem_ram, MAX_SUPERMEM_SIZE + 1);
  trs_load_uint8(file, rom, MAX_ROM_SIZE + 1);
  trs_load_uint8(file, cp500_rom, CP500_ROM_SIZE + 1);
  trs_load_uint8(file, video, MAX_VIDEO_SIZE + 1);
  trs_load_int(file, &trs_rom_size, 1);
  trs_load_int(file, &trs_video_size, 1);
  trs_load_int(file, &memory_map, 1);
  trs_load_int(file, bank_offset, 2);
  trs_load_int(file, &video_offset, 1);
  trs_load_int(file, &romin, 1);
  trs_load_uint32(file, &bank_base, 1);
  trs_load_uint8(file, &mem_command, 1);
  trs_load_int(file, &huffman_ram, 1);
  trs_load_int(file, &hypermem, 1);
  trs_load_int(file, &supermem, 1);
  trs_load_int(file, &selector, 1);
  trs_load_int(file, &selector_reg, 1);
  trs_load_int(file, &m_a11_flipflop, 1);
  trs_load_int(file, &eg3200, 1);
  trs_load_int(file, &system_byte, 1);
}

