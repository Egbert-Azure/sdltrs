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
 * Debug flags.  Update help_message in debug.c if these change.
 */
#define IODEBUG_IN  (1 << 0)  /* IN instructions */
#define IODEBUG_OUT (2 << 0)  /* OUT instructions */

#include <time.h>

#include "error.h"
#include "trs.h"
#include "trs_disk.h"
#include "trs_hard.h"
#include "trs_state_save.h"
#include "trs_stringy.h"
#include "trs_uart.h"

int trs_io_debug_flags;

static int modesel;         /* Model I */
static int modeimage = 0x8; /* Model III/4/4p */
static int ctrlimage;       /* Model 4/4p & M6845 */
static int rominimage;      /* Model 4p */

/* RTC addr/data */
static int rtc_reg;

/* M6845 CRT */
static int cursor_csr;
static int cursor_pos;
static int cursor_vis;

static void m6845_crt(int value)
{
  switch (ctrlimage) {
    case 0x01: /* Chars displayed */
      m6845_screen(value <= 80 ? value : 0, 0, 0);
      break;
    case 0x06: /* Lines displayed */
      m6845_screen(0, value <= 32 ? value : 0, 0);
      break;
    case 0x09: /* Maximum Raster address */
      m6845_screen(0, 0, value < 16 ? (value + 1) : 0);
      m6845_cursor(cursor_pos, cursor_csr, cursor_vis);
      break;
    case 0x0A: /* Cursor visible / Cursor Start Line */
      cursor_vis = !(value & (1 << 5)) || (value & (1 << 6));
      cursor_csr = value & 0x0F;
      m6845_cursor(cursor_pos, cursor_csr, cursor_vis);
      break;
    case 0x0E: /* Cursor LSB */
      if (cursor_vis)
        m6845_cursor(cursor_pos, 0, 0);
      cursor_pos = ((value & 0x3F) << 8) | (cursor_pos & 0x00FF);
      break;
    case 0x0F: /* Cursor MSB */
      cursor_pos = ((value & 0xFF) << 0) | (cursor_pos & 0xFF00);
      if (cursor_vis)
        m6845_cursor(cursor_pos, cursor_csr, 1);
      break;
  }
}

/*ARGSUSED*/
void z80_out(int port, int value)
{
  if (trs_io_debug_flags & IODEBUG_OUT) {
    debug("out (0x%02x), 0x%02x; pc 0x%04x\n", port, value, z80_state.pc.word);
  }

  /* EG 3200 Genie III & TCS Genie IIIs */
  if (eg3200 || genie3s) {
    switch (port) {
      case 0x48:
      case 0x50:
        trs_hard_out(TRS_HARD_DATA, value);
        break;
      case 0x49:
      case 0x51:
        trs_hard_out(TRS_HARD_PRECOMP, value);
        break;
      case 0x4A:
      case 0x52:
        trs_hard_out(TRS_HARD_SECCNT, value);
        break;
      case 0x4B:
      case 0x53:
        trs_hard_out(TRS_HARD_SECNUM, value);
        break;
      case 0x4C:
      case 0x54:
        trs_hard_out(TRS_HARD_CYLLO, value);
        break;
      case 0x4D:
      case 0x55:
        trs_hard_out(TRS_HARD_CYLHI, value);
        break;
      case 0x4E:
      case 0x56:
        trs_hard_out(TRS_HARD_SDH, value);
        break;
      case 0x4F:
      case 0x57:
        trs_hard_out(TRS_HARD_COMMAND, value);
        break;
      case 0x5B:
        if (genie3s)
          rtc_reg = value;
        break;
      case 0xE0:
        if (eg3200)
          rtc_reg = value;
        else
          trs_disk_select_write(value);
        break;
      case 0xE1:
      case 0xE2:
      case 0xE3:
        trs_disk_select_write(value);
        break;
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
        if (genie3s)
          trs_printer_write(value);
        break;
      case 0xEC:
        if (genie3s)
          trs_disk_command_write(value);
        break;
      case 0xED:
        if (genie3s)
          trs_disk_track_write(value);
        break;
      case 0xEE:
        if (genie3s)
          trs_disk_sector_write(value);
        break;
      case 0xEF:
        if (genie3s)
          trs_disk_data_write(value);
        break;
      case 0xF1:
        modeimage = value;
        break;
      case 0xF5:
        if (eg3200)
          trs_screen_inverse(value & 1);
        break;
      case 0xF6:
        ctrlimage = value;
        break;
      case 0xF7:
        m6845_crt(value);
        break;
      case 0xF9:
        genie3s_bank_out(0x100 | value);
        break;
      case 0xFA:
        if (genie3s)
          genie3s_sys_out(value);
        else
          eg3200 = value;
        break;
      case 0xFD:
        trs_printer_write(value);
        break;
      case 0xFE:
      case 0xFF:
        modesel = (value >> 3) & 1;
        trs_screen_expanded(modesel);
        trs_cassette_motor((value >> 2) & 1);
        trs_cassette_out(value & 0x3);
        break;
      default:
        break;
    }
    return;
  }

  /* Ports common to all TRS-80 models */
  switch (port) {
  case TRS_HARD_WP:       /* 0xC0 */
    if (trs_model == 1 && lubomir) {
      lsb_bank_out(value);
      return;
    }
    /* Fall through */
  case TRS_HARD_CONTROL:  /* 0xC1 */
  case TRS_HARD_DATA:     /* 0xC8 */
  case TRS_HARD_ERROR:    /* 0xC9 */ /*=TRS_HARD_PRECOMP*/
  case TRS_HARD_SECCNT:   /* 0xCA */
  case TRS_HARD_SECNUM:   /* 0xCB */
  case TRS_HARD_CYLLO:    /* 0xCC */
  case TRS_HARD_CYLHI:    /* 0xCD */
  case TRS_HARD_SDH:      /* 0xCE */
  case TRS_HARD_STATUS:   /* 0xCF */ /*=TRS_HARD_COMMAND*/
    trs_hard_out(port, value);
    break;
  case TRS_UART_RESET:    /* 0xE8 */
    trs_uart_reset_out(value);
    break;
  case TRS_UART_BAUD:     /* 0xE9 */
    trs_uart_baud_out(value);
    break;
  case TRS_UART_CONTROL:  /* 0xEA */
    trs_uart_control_out(value);
    break;
  case TRS_UART_DATA:     /* 0xEB */
    trs_uart_data_out(value);
    break;
  case 0x43: /* Alpha Technologies SuperMem */
    if (trs_model < 4 && supermem)
      mem_bank_base(value);
    break;
  }

  if (trs_model == 1) {
    /* Next, Model I only */
    switch (port) {
    case 0x00: /* HRG off */
    case 0x01: /* HRG on */
      hrg_onoff(port);
      break;
    case 0x02: /* HRG write address low byte */
      hrg_write_addr(value, 0xff);
      break;
    case 0x03: /* HRG write address high byte */
      hrg_write_addr(value << 8, 0x3f00);
      break;
    case 0x05: /* HRG write data byte */
      hrg_write_data(value);
      break;
      /* Selector doesn't decode A5 */
    case 0x1F:
    case 0x3F:
      if (selector)
        selector_out(value);
      break;
    case 0xB5: /* Orchestra-85 right channel */
      trs_orch90_out(2, value);
      break;
    case 0xB9: /* Orchestra-85 left channel */
      trs_orch90_out(1, value);
      break;
    case 0xD0:
    case 0x10: /* Homebrew 80*22 SYS80.SYS */
      if (speedup <= 4)
        ctrlimage = value;
      break;
    case 0xD1:
    case 0x11: /* Homebrew 80*22 SYS80.SYS */
      if (speedup <= 4)
        m6845_crt(value);
      break;
    case 0xD2:
      if (speedup <= 4)
        s80z_out(value);
      break;
    case 0xDF:
      if (speedup <= 3 && lubomir == 0)
        eg64_mba_out(value);
      break;
    case 0xEC:
      if (lowe_le18)
        lowe_le18_write_data(value);
      else if (speedup == 3) /* Seatronics Super Speed-Up */
        trs_timer_speed(value);
      break;
    case 0xED:
      lowe_le18_write_x(value);
      break;
    case 0xEE:
      lowe_le18_write_y(value);
      break;
    case 0xEF:
      lowe_le18_write_control(value);
      break;
    case 0xF0:
    case 0xF1:
    case 0xF2:
    case 0xF3:
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
      if (stringy)
        stringy_out(port & 7, value);
      break;
    case 0xF8:
      trs_uart_data_out(value);
      break;
    case 0xF9:
      if (speedup < 4)
        genie3s_init_out(value);
      break;
    case 0xFA:
      if (speedup < 4)
        eg3200_init_out(value);
      break;
    case 0xFD:
      /* GENIE location of printer port */
      trs_printer_write(value);
      break;
    case 0xFE:
      /* Typical location for clock speedup kits */
      if (speedup) {
        if (speedup < 4)
          trs_timer_speed(value);
        else
          sys_byte_out(value);
      }
      break;
    case 0xFF:
      /* screen mode select is on D3 line */
      modesel = (value >> 3) & 1;
      trs_screen_expanded(modesel);
      /* do cassette emulation */
      trs_cassette_motor((value >> 2) & 1);
      trs_cassette_out(value & 0x3);
      /* Lubomir Bits 7 - 5 for EG 64.1 */
      if (lubomir)
        lsb_bank_out(value & 0xE0);
      break;
    default:
      break;
    }

  } else {
    /* Next, Models III/4/4P only */
    switch (port) {
    case 0x50: /* MegaMem memory slot */
    case 0x51:
    case 0x52:
    case 0x60:
    case 0x61:
    case 0x62:
      if (megamem)
        megamem_out(port & 0x0F, value);
      break;
    case 0x5f: /* Sprinter III */
      if (trs_model == 3)
          trs_timer_speed(value);
      break;
    case 0x75: /* Orchestra-90 right channel */
      trs_orch90_out(2, value);
      break;
    case 0x79: /* Orchestra-90 left channel */
      trs_orch90_out(1, value);
      break;
    case 0x80:
      if (trs_model >= 3) grafyx_write_x(value);
      break;
    case 0x81:
      if (trs_model >= 3) grafyx_write_y(value);
      break;
    case 0x82:
      if (trs_model >= 3) grafyx_write_data(value);
      break;
    case 0x83:
      if (trs_model >= 3) grafyx_write_mode(value);
      break;
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
      if (trs_model >= 4) {
	int changes = value ^ ctrlimage;
	if (changes & 0x80) {
	  mem_video_page((value & 0x80) >> 7);
	}
	if (changes & 0x70) {
	  mem_bank((value & 0x70) >> 4);
	}
	if (changes & 0x08) {
	  trs_screen_inverse((value & 0x08) >> 3);
	}
	if (changes & 0x04) {
	  trs_screen_80x24((value & 0x04) >> 2);
	}
	if (changes & 0x03) {
	  mem_map(value & 0x03);
	}
	ctrlimage = value;
      }
      break;
    case 0x8c:
      if (trs_model >= 4) grafyx_write_xoffset(value);
      break;
    case 0x8d:
      if (trs_model >= 4) grafyx_write_yoffset(value);
      break;
    case 0x8e:
      if (trs_model >= 4) grafyx_write_overlay(value);
      break;
    case 0x90:
      /* HyperMem uses bits 4-1 of this port, 0 is the existing
         sound */
      if (trs_model >= 4 && hypermem)
        mem_bank_base(value);
      /* Fall through - we affect the sound as well */
    case 0x91:
    case 0x92:
    case 0x93:
      trs_sound_out(value & 1);
      break;
    case 0x94:			/* Huffman memory expansion */
      if (trs_model >= 4 && huffman)
        mem_bank_base(value);
      break;
    case 0x9C:
    case 0x9D: /* !!? */
    case 0x9E: /* !!? */
    case 0x9F: /* !!? */
      if (trs_model == 5 /*4p*/) {
	rominimage = value & 1;
	mem_romin(rominimage);
      }
      break;
    case 0xE0:
    case 0xE1:
    case 0xE2:
    case 0xE3:
      trs_interrupt_mask_write(value);
      break;
    case TRSDISK3_INTERRUPT: /* 0xE4 */
    case 0xE5:
    case 0xE6:
    case 0xE7:
      trs_nmi_mask_write(value);
      break;
    case 0xEC:
    case 0xED:
    case 0xEE:
    case 0xEF:
      modeimage = value;
      /* cassette motor is on D1 */
      trs_cassette_motor((modeimage & 0x02) >> 1);
      /* screen mode select is on D2 */
      trs_screen_expanded((modeimage & 0x04) >> 2);
      /* alternate char set is on D3 */
      trs_screen_alternate(!((modeimage & 0x08) >> 3));
      /* clock speed is on D6; it affects timer HZ too */
      trs_timer_speed(modeimage);
      break;
    case TRSDISK3_COMMAND: /* 0xF0 */
      trs_disk_command_write(value);
      break;
    case TRSDISK3_TRACK: /* 0xF1 */
      trs_disk_track_write(value);
      break;
    case TRSDISK3_SECTOR: /* 0xF2 */
      trs_disk_sector_write(value);
      break;
    case TRSDISK3_DATA: /* 0xF3 */
      trs_disk_data_write(value);
      break;
    case TRSDISK3_SELECT: /* 0xF4 */
    case 0xF5:
    case 0xF6:
    case 0xF7:
      /* This should cause a 1-2us wait in T states... */
      trs_disk_select_write(value);
      break;
    case 0xF8:
    case 0xF9:
    case 0xFA:
    case 0xFB:
      trs_printer_write(value);
      break;
    case 0xFC:
    case 0xFD:
    case 0xFE:
    case 0xFF:
      if (trs_model == 3 && (value & 0x20) && grafyx_get_microlabs()) {
	/* do Model III Micro-Labs graphics card */
	grafyx_m3_write_mode(value);
      } else {
	/* do cassette emulation */
	trs_cassette_out(value & 3);
      }
      break;
    default:
      break;
    }
  }
}

/*ARGSUSED*/
int z80_in(int port)
{
  int value = 0xff; /* value returned for nonexistent ports */

  /* Support for a special HW real-time clock (TimeDate80?)
   * I used to have.  It was a small card-edge unit with a
   * battery that held the time/date with power off.
   * - Joe Peterson (joe@skyrush.com)
   *
   * According to the LDOS Quarterly 1-6, TChron1, TRSWatch, and
   * TimeDate80 are accessible at high ports 0xB0-0xBC, while
   * T-Timer is accessible at high ports 0xC0-0xCC.  It does
   * not say where the low ports were; Joe's code had 0x70-0x7C,
   * so I presume that's correct at least for the TimeDate80.
   * Newclock-80 (by Alpha Products) uses 0x70-0x7C or 0xB0-0xBC.
   * Note: 0xC0-0xCC conflicts with Radio Shack hard disk, so
   * clock access at these ports is disabled starting in xtrs 4.1.
   *
   * These devices were based on the MSM5832 chip, which returns only
   * a 2-digit year.  It's not clear what software will do with the
   * date in years beyond 1999.
   */

  if ((port >= 0x70 && port <= 0x7C)
      || (port >= 0x68 && port <= 0x6D)
      || (port >= 0xB0 && port <= 0xBC)
      || (port == 0xE0 && eg3200)
      || (port == 0x5A && genie3s)) {
    time_t time_secs = time(NULL);
    struct tm *time_info = localtime(&time_secs);

    /* Ports in David Keil's TRS-80 Emulator */
    if (port >= 0x68 && port <= 0x6D) {
      switch (port) {
        case 0x68:
          value = time_info->tm_sec;
          break;
        case 0x69:
          value = time_info->tm_min;
          break;
        case 0x6A:
          value = time_info->tm_hour;
          break;
        case 0x6B:
          value = (time_info->tm_year + 1900) % 100;
          break;
        case 0x6C:
          value = time_info->tm_mday;
          break;
        case 0x6D:
          value = (time_info->tm_mon) + 1;
          break;
      }
      /* BCD value */
      value = (value / 10 * 16 + value % 10);
      goto done;
    }

    if (eg3200 || genie3s)
      port = (rtc_reg >> 4);

    switch (port & 0x0F) {
    case 0xC: /* year (high) */
      value = (time_info->tm_year / 10) % 10;
      goto done;
    case 0xB: /* year (low) */
      value = (time_info->tm_year % 10);
      goto done;
    case 0xA: /* month (high) */
      value = ((time_info->tm_mon + 1) / 10);
      goto done;
    case 0x9: /* month (low) */
      value = ((time_info->tm_mon + 1) % 10);
      goto done;
    case 0x8: /* date (high) and leap year (bit 2) */
      value = ((time_info->tm_mday / 10) | ((time_info->tm_year % 4) ? 0 : 4));
      goto done;
    case 0x7: /* date (low) */
      value = (time_info->tm_mday % 10);
      goto done;
    case 0x6: /* day-of-week */
      value = time_info->tm_wday;
      goto done;
    case 0x5: /* hours (high) and PM (bit 2) and 24hr (bit 3) */
      value = ((time_info->tm_hour / 10) | 8);
      goto done;
    case 0x4: /* hours (low) */
      value = (time_info->tm_hour % 10);
      goto done;
    case 0x3: /* minutes (high) */
      value = (time_info->tm_min / 10);
      goto done;
    case 0x2: /* minutes (low) */
      value = (time_info->tm_min % 10);
      goto done;
    case 0x1: /* seconds (high) */
      value = (time_info->tm_sec / 10);
      goto done;
    case 0x0: /* seconds (low) */
      value = (time_info->tm_sec % 10);
      goto done;
    }
  }

  /* EG 3200 Genie III & TCS Genie IIIs */
  if (eg3200 || genie3s) {
    switch (port) {
      case 0x48:
      case 0x50:
        value = trs_hard_in(TRS_HARD_DATA);
        break;
      case 0x49:
      case 0x51:
        value = trs_hard_in(TRS_HARD_ERROR);
        break;
      case 0x4A:
      case 0x52:
        value = trs_hard_in(TRS_HARD_SECCNT);
        break;
      case 0x4B:
      case 0x53:
        value = trs_hard_in(TRS_HARD_SECNUM);
        break;
      case 0x4C:
      case 0x54:
        value = trs_hard_in(TRS_HARD_CYLLO);
        break;
      case 0x4D:
      case 0x55:
        value = trs_hard_in(TRS_HARD_CYLHI);
        break;
      case 0x4E:
      case 0x56:
        value = trs_hard_in(TRS_HARD_SDH);
        break;
      case 0x4F:
      case 0x57:
        value = trs_hard_in(TRS_HARD_STATUS);
        break;
      case 0xE0:
      case 0xE1:
      case 0xE2:
      case 0xE3:
        value = trs_interrupt_latch_read();
        break;
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
        if (genie3s)
          value = trs_printer_read();
        break;
      case 0xEC:
        if (genie3s)
          value = trs_disk_status_read();
        break;
      case 0xED:
        if (genie3s)
          value = trs_disk_track_read();
        break;
      case 0xEE:
        if (genie3s)
          value = trs_disk_sector_read();
        break;
      case 0xEF:
        if (genie3s)
          value = trs_disk_data_read();
        break;
      case 0xF1:
        value = modeimage;
        break;
      case 0xF7:
        switch (ctrlimage) {
          case 0x0E: /* Cursor LSB */
            value = (cursor_pos >> 8) & 0xFF;
            break;
          case 0x0F: /* Cursor MSB */
            value = (cursor_pos >> 0) & 0xFF;
            break;
        }
        break;
      case 0xF9:
        value = genie3s & 0xFF;
        break;
      case 0xFA:
        value = sys_byte_in();
        break;
      case 0xFD:
        value = trs_printer_read();
        break;
      case 0xFE:
      case 0xFF:
        value = (!modesel ? 0x7f : 0x3f) | trs_cassette_in();
        break;
      default:
        break;
    }
    goto done;
  }

  /* Ports common to all TRS-80 models */
  switch (port) {
  case 0x00:
    value = trs_joystick_in();
    goto done;
  case TRS_HARD_WP:       /* 0xC0 */
  case TRS_HARD_CONTROL:  /* 0xC1 */
  case TRS_HARD_DATA:     /* 0xC8 */
  case TRS_HARD_ERROR:    /* 0xC9 */ /*=TRS_HARD_PRECOMP*/
  case TRS_HARD_SECCNT:   /* 0xCA */
  case TRS_HARD_SECNUM:   /* 0xCB */
  case TRS_HARD_CYLLO:    /* 0xCC */
  case TRS_HARD_CYLHI:    /* 0xCD */
  case TRS_HARD_SDH:      /* 0xCE */
  case TRS_HARD_STATUS:   /* 0xCF */ /*=TRS_HARD_COMMAND*/
    value = trs_hard_in(port);
    goto done;
  case TRS_UART_MODEM:    /* 0xE8 */
    value = trs_uart_modem_in();
    goto done;
  case TRS_UART_SWITCHES: /* 0xE9 */
    value = trs_uart_switches_in();
    goto done;
  case TRS_UART_STATUS:   /* 0xEA */
    value = trs_uart_status_in();
    goto done;
  case TRS_UART_DATA:     /* 0xEB */
    value = trs_uart_data_in();
    goto done;
  case 0x43: /* Supermem memory expansion */
    if (trs_model < 4 && supermem) {
      value = mem_read_bank_base();
      goto done;
    }
  }

  if (trs_model == 1) {
    /* Model I only */
    switch (port) {
#if 0 /* Conflicts with joystick port */
    case 0x00: /* HRG off (undocumented) */
#endif
    case 0x01: /* HRG on (undocumented) */
      hrg_onoff(port);
      goto done;
    case 0x04: /* HRG read data byte */
      value = hrg_read_data();
      goto done;
    case 0xDF:
      if (lubomir == 0) {
        eg64_mba_out(7);
        value = 0;
      }
      goto done;
    case 0xEC:
      value = lowe_le18_read();
      goto done;
    case 0xF0:
    case 0xF1:
    case 0xF2:
    case 0xF3:
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
      if (stringy)
        value = stringy_in(port & 7);
      goto done;
    case 0xF9:
      value = trs_uart_data_in();
      goto done;
    case 0xFD:
      /* GENIE location of printer port */
      value = trs_printer_read();
      goto done;
    case 0xFE:
      if (speedup >= 4)
        value = sys_byte_in();
      goto done;
    case 0xFF:
      value = (!modesel ? 0x7f : 0x3f) | trs_cassette_in();
      goto done;
    }

  } else {
    /* Models III/4/4P only */
    switch (port) {
    case 0x82:
      if (trs_model >= 3) {
	value = grafyx_read_data();
	goto done;
      }
      break;
    case 0x94: /* Huffman memory expansion */
      value = mem_read_bank_base();
      goto done;
    case 0x9C: /* !!? */
    case 0x9D: /* !!? */
    case 0x9E: /* !!? */
    case 0x9F: /* !!? */
      if (trs_model == 5 /*4p*/) {
	value = rominimage;
	goto done;
      }
      break;
    case 0xE0:
    case 0xE1:
    case 0xE2:
    case 0xE3:
      value = trs_interrupt_latch_read();
      goto done;
    case 0xEC:
    case 0xED:
    case 0xEE:
    case 0xEF:
      trs_timer_interrupt(0); /* acknowledge */
      value = 0xFF;
      goto done;
    case TRSDISK3_INTERRUPT: /* 0xE4 */
      value = trs_nmi_latch_read();
      goto done;
    case TRSDISK3_STATUS: /* 0xF0 */
      value = trs_disk_status_read();
      goto done;
    case TRSDISK3_TRACK: /* 0xF1 */
      value = trs_disk_track_read();
      goto done;
    case TRSDISK3_SECTOR: /* 0xF2 */
      value = trs_disk_sector_read();
      goto done;
    case TRSDISK3_DATA: /* 0xF3 */
      value = trs_disk_data_read();
      goto done;
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
      value = cp500_a11_flipflop_toggle();
      goto done;
    case 0xF8:
    case 0xF9:
    case 0xFA:
    case 0xFB:
      value = trs_printer_read() | (ctrlimage & 0x0F);
      goto done;
    case 0xFC:
    case 0xFD:
    case 0xFF:
      value = (modeimage & 0x7e) | trs_cassette_in();
      goto done;
    }
  }

 done:
  if (trs_io_debug_flags & IODEBUG_IN) {
    debug("in (0x%02x) => 0x%02x; pc %04x\n", port, value, z80_state.pc.word);
  }

  return value;
}

void trs_io_save(FILE *file)
{
  trs_save_int(file, &modesel, 1);
  trs_save_int(file, &modeimage, 1);
  trs_save_int(file, &ctrlimage, 1);
  trs_save_int(file, &rominimage, 1);
  trs_save_int(file, &cursor_csr, 1);
  trs_save_int(file, &cursor_pos, 1);
  trs_save_int(file, &cursor_vis, 1);
  trs_save_int(file, &rtc_reg, 1);
}

void trs_io_load(FILE *file)
{
  trs_load_int(file, &modesel, 1);
  trs_load_int(file, &modeimage, 1);
  trs_load_int(file, &ctrlimage, 1);
  trs_load_int(file, &rominimage, 1);
  trs_load_int(file, &cursor_csr, 1);
  trs_load_int(file, &cursor_pos, 1);
  trs_load_int(file, &cursor_vis, 1);
  trs_load_int(file, &rtc_reg, 1);
}

