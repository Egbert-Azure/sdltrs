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
 * Portions copyright (c) 1996-2020, Timothy P. Mann
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
 * trs.h
 */
#ifndef _TRS_H
#define _TRS_H

#include <SDL_types.h>
#include "z80.h"

#define STRETCH_AMOUNT 4000
#define DEFAULT_SAMPLE_RATE 44100  /* samples/sec to use for .wav files */

#if defined(__OS2__) || defined(_WIN32)
#define DIR_SLASH '\\'
#else
#define DIR_SLASH '/'
#endif

extern char romfile[FILENAME_MAX];
extern char romfile3[FILENAME_MAX];
extern char romfile4p[FILENAME_MAX];
extern char trs_hard_dir[FILENAME_MAX];
extern char trs_cass_dir[FILENAME_MAX];
extern char trs_disk_dir[FILENAME_MAX];
extern char trs_disk_set_dir[FILENAME_MAX];
extern char trs_state_dir[FILENAME_MAX];
extern char trs_printer_dir[FILENAME_MAX];
extern char trs_printer_command[FILENAME_MAX];
extern char trs_cmd_file[FILENAME_MAX];
extern char trs_config_file[FILENAME_MAX];
extern char trs_state_file[FILENAME_MAX];

extern int trs_model; /* 1, 3, 4, 5(=4p) */
extern int eg3200;    /* EACA EG 3200: Genie III */
extern int genie3s;   /* TCS Genie IIIs */
extern unsigned int foreground;
extern unsigned int background;
extern unsigned int gui_foreground;
extern unsigned int gui_background;
extern int fullscreen;
extern int trs_emu_mouse;

extern int trs_continuous; /* 1= run continuously,
			      0= enter debugger after instruction,
			     -1= suppress interrupt and enter debugger */
extern int trs_disk_debug_flags;
extern int trs_io_debug_flags;
extern int trs_emtsafe;

extern void trs_parse_command_line(int argc, char **argv, int *debug);
extern int trs_write_config_file(const char *filename);
extern int trs_load_cmd(const char *filename);
extern int trs_load_config_file(void);

extern void trs_screen_init(void);
extern void screen_init(void);
extern void trs_rom_init(void);
extern void trs_screen_write_char(unsigned int position, Uint8 char_index);
extern void trs_screen_update(void);
extern void trs_screen_expanded(int flag);
extern void trs_screen_alternate(int flag);
extern void trs_screen_80x24(int flag);
extern void trs_screen_inverse(int flag);
extern void trs_screen_refresh(void);
extern void trs_screen_var_reset(void);
extern void trs_screen_caption(void);

extern void trs_disk_led(int drive, int on_off);
extern void trs_hard_led(int drive, int on_off);
extern void trs_turbo_led(void);

extern void trs_reset(int poweron);
extern void trs_exit(int confirm);
extern void trs_sdl_cleanup(void);

extern void trs_kb_reset(void);
extern void trs_kb_bracket(int shifted);
extern int trs_kb_mem_read(int address);
extern void trs_kb_heartbeat(void);
extern void trs_xlate_keysym(int keysym);
extern void clear_key_queue(void);
extern int stretch_amount;
extern int trs_kb_bracket_state;

extern int scale;
extern int resize3;
extern int resize4;
extern int trs_uart_switches;
extern int trs_show_led;
extern int window_border_width;
extern int trs_hd_boot;
extern int trs_joystick_num;
extern int trs_keypad_joystick;
extern int trs_charset1;
extern int trs_charset3;
extern int trs_charset4;
extern int trs_paused;
extern int trs_printer;
extern int trs_sound;

extern void trs_get_event(int wait);
extern void trs_sdl_flush(void);

extern void trs_printer_write(int value);
extern int trs_printer_read(void);
extern int trs_printer_reset(void);

extern void trs_cassette_motor(int value);
extern void trs_cassette_out(int value);
extern int trs_cassette_in(void);
extern void trs_sound_out(int value);

extern int trs_joystick_in(void);

extern int trs_rom_size;

extern Uint8 trs_interrupt_latch_read(void);
extern Uint8 trs_nmi_latch_read(void);
extern void trs_interrupt_mask_write(Uint8);
extern void trs_nmi_mask_write(Uint8);
extern void trs_reset_button_interrupt(int state);
extern void trs_disk_intrq_interrupt(int state);
extern void trs_disk_drq_interrupt(int state);
extern void trs_disk_motoroff_interrupt(int state);
extern void trs_uart_err_interrupt(int state);
extern void trs_uart_rcv_interrupt(int state);
extern void trs_uart_snd_interrupt(int state);
extern void trs_timer_interrupt(int state);
extern void trs_timer_init(void);
extern void trs_timer_off(void);
extern void trs_timer_on(void);
extern void trs_timer_cycles(void);
extern void trs_timer_speed(int flag);
extern void trs_timer_sync_with_host(void);
extern void trs_turbo_mode(int mode);
extern void trs_cassette_rise_interrupt(int dummy);
extern void trs_cassette_fall_interrupt(int dummy);
extern void trs_cassette_clear_interrupts(void);
extern int trs_cassette_interrupts_enabled(void);
extern void trs_cassette_update(int dummy);
extern int cassette_default_sample_rate;
extern void trs_orch90_out(int chan, int value);
extern void trs_cassette_reset(void);
extern void assert_state_void(int dummy);
extern void transition_out(int dummy);
extern void trs_cassette_kickoff(int dummy);
extern void orch90_flush(int dummy);
extern void trs_disk_lostdata(int dummy);
extern void trs_disk_done(int dummy);
extern void trs_disk_firstdrq(int dummy);
extern void trs_uart_set_avail(int dummy);
extern void trs_uart_set_empty(int dummy);

extern void trs_disk_debug(void);
extern int trs_disk_motoroff(void);

extern int huffman_ram;
extern int hypermem;
extern int supermem;
extern int selector;

extern void selector_out(Uint8);

extern int lowercase;
extern int lubomir;
extern int stringy;

extern void mem_video_page(int which);
extern void mem_bank(int which);
extern void mem_map(int which);
extern void mem_bank_base(int bits);
extern int mem_read_bank_base(void);
extern void mem_romin(int state);
extern int cp500_a11_flipflop_toggle(void);
extern void eg3200_init_out(int value);
extern void genie3s_bank_out(int value);
extern void genie3s_init_out(int value);
extern void genie3s_sys_out(int value);
extern void genie3s_char(int index, int address, int byte);
extern void genie3s_hrg(int value);
extern void genie3s_hrg_write(int position, int byte);
extern Uint8 genie3s_hrg_read(int position);
extern void m6845_cursor(int position, int line, int visible);
extern void m6845_screen(int chars, int lines);

extern void eg64_mba_out(int value);
extern void lsb_bank_out(int value);
extern void sys_byte_out(int value);
extern int  sys_byte_in(void);

extern void trs_debug(void);

typedef void (*trs_event_func)(int arg);
void trs_schedule_event(trs_event_func f, int arg, int tstates);
void trs_schedule_event_us(trs_event_func f, int arg, int us);
void trs_do_event(void);
void trs_cancel_event(void);
trs_event_func trs_event_scheduled(void);

void grafyx_write_x(int value);
void grafyx_write_y(int value);
void grafyx_write_data(int value);
int grafyx_read_data(void);
void grafyx_write_mode(int value);
void grafyx_write_xoffset(int value);
void grafyx_write_yoffset(int value);
void grafyx_write_overlay(int value);
void grafyx_set_microlabs(int on_off);
int grafyx_get_microlabs(void);
void grafyx_m3_reset(void);
void grafyx_m3_write_mode(int value);
Uint8 grafyx_m3_read_byte(int position);
int grafyx_m3_write_byte(int position, int value);
void hrg_onoff(int enable);
void hrg_write_addr(int addr, int mask);
void hrg_write_data(int data);
int hrg_read_data(void);
extern int lowe_le18;
void lowe_le18_write_x(int);
void lowe_le18_write_y(int);
int lowe_le18_read(void);
void lowe_le18_write_data(int);
void lowe_le18_write_control(int);

void trs_get_mouse_pos(int *x, int *y, unsigned int *buttons);
void trs_set_mouse_pos(int x, int y);
void trs_get_mouse_max(int *x, int *y, unsigned int *sens);
void trs_set_mouse_max(int x, int y, unsigned int sens);
int trs_get_mouse_type(void);

extern int timer_hz;
extern int timer_overclock_rate;
extern int timer_overclock;
extern int speedup;
extern float clock_mhz_1;
extern float clock_mhz_3;
extern float clock_mhz_4;

int put_twobyte(Uint16 n, FILE* f);
int put_fourbyte(Uint32 n, FILE* f);
int get_twobyte(Uint16 *n, FILE* f);
int get_fourbyte(Uint32 *n, FILE* f);


#endif /*_TRS_H*/
