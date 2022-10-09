#include "error.h"
#include "trs.h"
#include "trs_clones.h"
#include "trs_cp500.h"
#include "trs_memory.h"
#include "trs_state_save.h"

/**********************************************************************
 * CP-500 - Model III clone from Brazil, made by Prologica.
 *
 * There were 4 models (each one backwards compatible with previous):
 * - CP-500 (original):
 *   - 16K of ROM instead of 14K.
 *   - 14K of ROM is identical to Model III, expect for
 *     a few patched bytes
 *   - 2K extra ROM contain the "resident monitor" which can be bank
 *     switched to appear on 3000-37FF.
 * - CP-500 M80:
 *   - Contains additional logic board.
 *   - Can bank switch to 64K of RAM in order to support CP/M.
 *   - Supports 80x24 video mode (only in "CP/M mode").
 *   - Design together with SO-08, which is the CP/M clone from Pologica
 *     made to run in this computer.
 * - CP-500 M80c (compact version)
 *   - More compact version with slim drives and revised internals.
 *   - No changes in terms of emulation.
 * - CP-500 Turbo (4 MHz clock)
 *   - Not emulated yet.
 */
/*#define DEBUG_CP500 1*/

static int cp500_model;

static struct {
  int video_first_row;
} cp500_m80;

void cp500_reset_mode() {
  cp500_model = 0;
  mem_map(0);
}

/*
 * Switches CP-500 to a new mode. Mode switching in CP-500 is done by
 * reading (!) port F4, which is unused (when reading) in Model III.
 */
Uint8 cp500_switch_mode(int mode) {

  /*
   * In the original CP-500, mode switching is documented in the
   * technical manual. It looks like just reading the port is enough to
   * trigger a flip-flop. The CP-500 ROM only triggers it during
   * bootstrap, once to map the 2K of extra ROM into 3000-37FF,
   * then copies it into RAM, then triggers it again to restore the
   * memory map. The port is never touched again.
   *
   * The M80 does not seem to have a technical manual, and there is no
   * known documentation of how it switches mode. But by reverse
   * engineering SO-08 it was determined that it uses the same mechanism
   * as the predecessor model, i.e., reading from port F4, however
   * before switching a specific number is always put on register A
   * first. So the mode it will switch to depend on the contents of A
   * when port F4 is read. This works because this is what the IN
   * instruction does according to Zilog's Z-80 manual:
   *
   * > The operand n is placed on the bottom half (A0 through A7) of the
   * > address bus to select the I/O device at one of 256 possible ports.
   * > The contents of the Accumulator also appear on the top half
   * > (A8 through A15) of the address bus at this time.
   *
   * So the M80 circuitry is looking at the whole 16-bit in the address
   * bus, and switching mode accordingly.
   *
   * The values in this method were discovered by reverse engineering
   * the CP-500 ROM and SO-08. It is possible that the hardware is not
   * responding to specific values, but to specific bits. However, the
   * implementation of the emulator is using only the exact values used
   * by SO-08. As far as it is known, only SO-08 uses this
   * functionality.
   *
   * -- Leonardo Brondani Schenkel, 2022-08.
   */
  switch (mode) {

    /* CP-500 (all models): */

    case 0x00: /* Standard TRS-80 Model III memory map */
      mem_map(0);
      cp500_model = CP500;
      break;
    case 0x20: /* 3000-37FF points to extra 2K region in EPROM 4 */
      mem_map(1);
      cp500_model = CP500;
      break;

      /*
       * CP-500 M80 and later (CP/M compatibility):
       */

    case 0x1d: /* 64K RAM */
    case 0x1c: /* TODO: it is still unknown how this differs from 1D */
      mem_map(2);
      cp500_model = CP500_M80;
      break;

    case 0x05: /* 64K RAM, 80x24, video lines 1-8 mapped to RAM */
    case 0x45: /* 64K RAM, 80x24, video lines 9-15 mapped to RAM */
    case 0x85: /* 64K RAM, 80x24, video lines 15-24 mapped to RAM */
      mem_map(3);
      cp500_model = CP500_M80;
      /* Precalculate now to avoid doing on every memory access: */
      cp500_m80.video_first_row = mode >> 3;
      mem_video_page((mode >> 6) * 1024);
      trs_screen_80x24(1);
      break;

    default:
      fatal("CP-500: port=0xF4 A=0x%02x: unimplemented PC=%04x\n", Z80_A,
            Z80_PC - 2);
  }

  /*
   * Set the right quirks. While reverse engineering SO-08, it was
   * found that some behaviour of the emulator don't quite match the
   * expectations of the code. This tweaks the emulator to adapt the
   * behaviour only when CP-500 is being emulated.
   *
   * It is unknown if the original CP-500 and M80 behave the same since
   * only SO-08 is known to be affected, and it can only run in the M80.
   * So for now we enable the quirks only for M80.
   */

   trs_clone_quirks(cp500_model);

#ifdef DEBUG_CP500
  debug("CP-500: switched to mode A=%02x [PC=%04x]\n", Z80_A, Z80_PC);
#endif

  return 0; /* TODO: unknown what the hardware actually returns */
}

Uint8 cp500_mem_read(int address, int mem_map, Uint8 *rom, Uint8 *ram) {
  switch (mem_map) {
    case 0x31: /* 3000-37FF = extra 2K in EPROM 4 */
      if (address >= 0x3000 && address <= 0x3FFF) {
        return rom[address | 0x0800];
      }
      return trs80_model3_mem_read(address);

    case 0x32: /* 64K of RAM, nothing else mapped */
      return ram[address];

    case 0x33: /* 80x24, 3800 = keyboard, 3C00-3CFF = VRAM */
      if (address >= RAM_START) {
        return ram[address];
      } else if (address >= VIDEO_START) {
        return mem_video_page_read(address - VIDEO_START);
      } else if (address >= KEYBOARD_START) {
        return trs_kb_mem_read(address);
      }
  }
#ifdef DEBUG_CP500
  error("Invalid read of address %04x, returning FF [PC=%04x, mem_map=%02x]",
      address, Z80_PC, mem_map);
#endif
  return 0xFF;
}

void cp500_mem_write(int address, Uint8 value, int mem_map, Uint8 *ram) {
  switch (mem_map) {
    case 0x31: /* 3000-37FF = extra 2K in EPROM 4 */
      trs80_model3_mem_write(address, value);
      return;

    case 0x32: /* 64K of RAM, nothing else mapped */
      ram[address] = value;
      return;

    case 0x33: /* video and keyboard mapped  */
      if (address >= RAM_START) {
        ram[address] = value;
        return;
      } else if (address >= VIDEO_START) {
        address = address - VIDEO_START;
        if (mem_video_page_write(address, value)) {
          /*
           * This is a hack to allow using the Model 4 video code for now.
           *
           * In M80, the 80x24 modes use 3 banks of 128x8 characters.
           * each. That conveniently adds up to 1K per bank (the exact
           * address space in the original Model III memory map) and
           * simplifies the video circuitry. Anything beyond column 80
           * in each row is ignored.
           *
           * In Model 4, the video RAM is a linear address space of
           * 80 columns each.
           *
           * So we figure out the row/col of the byte in M80 addressing
           * rules, and convert that into the right offset in Model 4:
           */
          int col = address % 128;
          if (col < 80) {
            int row = address / 128 + cp500_m80.video_first_row;

            trs_screen_write_char(row * 80 + col, value);
          }
        }
      }
      return;
  }
#ifdef DEBUG_CP500
  error("Invalid write of address %04x [PC=%04x, mem_map=%02x]",
      address, Z80_PC, mem_map);
#endif
}

Uint8 *cp500_mem_addr(int address, int mem_map, Uint8 *rom, Uint8 *ram, int writing) {
  switch (mem_map) {
    case 0x31: /* 3000-37FF = extra 2K in EPROM 4 */
    case 0x39:
      if (address >= 0x3000 && address <= 0x3FFF && writing == 0) {
        return &rom[address | 0x0800];
      }
      return trs80_model3_mem_addr(address, writing);

    case 0x32: /* 64K of RAM, nothing else mapped */
    case 0x3A:
      return &ram[address];

    case 0x33: /* 80x24, 3800 = keyboard, 3C00-3CFF = VRAM */
    case 0x3B:
      if (address >= RAM_START) {
        return &ram[address];
      } else if (address >= VIDEO_START) {
        return mem_video_page_addr(address - VIDEO_START);
      }
  }
  return NULL;
}

void trs_cp500_save(FILE *file)
{
  trs_save_int(file, &cp500_model, 1);
  trs_save_int(file, &cp500_m80.video_first_row, 1);
}

void trs_cp500_load(FILE *file)
{
  trs_load_int(file, &cp500_model, 1);
  trs_load_int(file, &cp500_m80.video_first_row, 1);
}
