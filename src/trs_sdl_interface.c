/* SDLTRS version Copyright (c): 2006, Mark Grebe */

/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/

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
   Modified by Mark Grebe, 2006
   Last modified on Wed May 07 09:56:00 MST 2006 by markgrebe
*/

/*#define MOUSEDEBUG 1*/
/*#define XDEBUG 1*/
/*#define QDEBUG 1*/

/*
 * trs_sdl_interface.c
 *
 * SDL interface for TRS-80 Emulator
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <SDL.h>

#include "blit.h"
#include "error.h"
#include "trs_iodefs.h"
#include "trs.h"
#include "trs_disk.h"
#include "trs_stringy.h"
#include "trs_uart.h"
#include "trs_state_save.h"
#include "trs_sdl_gui.h"
#include "trs_cassette.h"
#include "trs_sdl_keyboard.h"

extern char trs_char_data[][MAXCHARS][TRS_CHAR_HEIGHT];

extern void trs_gui_keys_sdltrs(void);
extern void trs_gui_model(void);
extern int trs_timer_is_turbo(void);
extern int trs_timer_switch_turbo(void);

#define MAX_RECTS 2048
#define WHITE 0xe0e0ff
#define BLACK 0
#define GREEN 0x344843

/* Public data */
int window_border_width = 2;
unsigned int foreground = WHITE;
unsigned int background = 0;
unsigned int gui_foreground = WHITE;
unsigned int gui_background = GREEN;
int trs_show_led = 1;
int scale_x = 1;
int scale_y = 2;
char romfile[FILENAME_MAX];
char romfile3[FILENAME_MAX];
char romfile4p[FILENAME_MAX];
int fullscreen = 0;
int resize = 0;
int resize3 = 1;
int resize4 = 0;
int scanlines = 0;
int trs_paused = 0;
char trs_disk_dir[FILENAME_MAX] = "";
char trs_hard_dir[FILENAME_MAX] = "";
char trs_cass_dir[FILENAME_MAX] = "";
char trs_disk_set_dir[FILENAME_MAX] = "";
char trs_state_dir[FILENAME_MAX] = "";
char trs_printer_dir[FILENAME_MAX] = "";
char trs_cmd_file[FILENAME_MAX];
char trs_config_file[FILENAME_MAX];
char init_state_file[FILENAME_MAX];
char trs_printer_command[FILENAME_MAX];
int trs_charset = 3;
int trs_charset1 = 3;
int trs_charset3 = 4;
int trs_charset4 = 8;
int trs_emu_mouse = FALSE;

/* Private data */
static unsigned char trs_screen[2048];
static unsigned char trs_gui_screen[1024];
static unsigned char trs_gui_screen_invert[1024];
static unsigned char trs_gui_screen_copy[1024];
static unsigned char trs_gui_screen_invert_copy[1024];
static int cpu_panel = 0;
static int debugger = 0;
static int screen_chars = 1024;
static int row_chars = 64;
static int col_chars = 16;
static int border_width = 2;
static int text80x24 = 0, screen640x240 = 0;
static int drawnRectCount = 0;
static int top_margin = 0;
static int left_margin = 0;
static int led_width = 0;
static int currentmode = NORMAL;
static int OrigHeight,OrigWidth;
static int cur_char_width = TRS_CHAR_WIDTH;
static int cur_char_height = TRS_CHAR_HEIGHT * 2;
static int disksizes[8] = {5,5,5,5,8,8,8,8};
static int disksteps[8] = {1,1,1,1,2,2,2,2};
static int mousepointer = 1;
static int mouse_x_size = 640, mouse_y_size = 240;
static int mouse_sens = 3;
static int mouse_last_x = -1, mouse_last_y = -1;
static int mouse_old_style = 0;
static unsigned int mouse_last_buttons;
static SDL_Surface *trs_char[6][MAXCHARS];
static SDL_Surface *trs_box[3][64];
static SDL_Surface *image;
static SDL_Surface *screen;
static SDL_Rect drawnRects[MAX_RECTS];
#ifdef SDL2
static SDL_Window *window = NULL;
#endif
static Uint32 light_red;
static Uint32 bright_red;
static Uint32 light_orange;
static Uint32 bright_orange;
static Uint32 last_key[256];

#if defined(SDL2) || !defined(NOX)
#define PASTE_IDLE    0
#define PASTE_GETNEXT 1
#define PASTE_KEYDOWN 2
#define PASTE_KEYUP   3
static int paste_state = PASTE_IDLE;
static int paste_lastkey = FALSE;
extern int  PasteManagerStartPaste(void);
extern void PasteManagerStartCopy(char *string);
extern int PasteManagerGetChar(unsigned short *character);

#define COPY_OFF       0
#define COPY_IDLE      1
#define COPY_STARTED   2
#define COPY_DEFINED   3
#define COPY_CLEAR     4
static int copyStatus = COPY_IDLE;
static int selectionStartX = 0;
static int selectionStartY = 0;
static int selectionEndX = 0;
static int selectionEndY = 0;
static int requestSelectAll = FALSE;
#endif

/* Support for Micro Labs Grafyx Solution and Radio Shack hi-res card */

/* True size of graphics memory -- some is offscreen */
#define G_XSIZE 128
#define G_YSIZE 256
static char grafyx[(2*G_YSIZE*MAX_SCALE) * (G_XSIZE*MAX_SCALE)];
static unsigned char grafyx_unscaled[G_YSIZE][G_XSIZE];

static unsigned char grafyx_microlabs = 0;
static unsigned char grafyx_x = 0, grafyx_y = 0, grafyx_mode = 0;
static unsigned char grafyx_enable = 0;
static unsigned char grafyx_overlay = 0;
static unsigned char grafyx_xoffset = 0, grafyx_yoffset = 0;

/* Port 0x83 (grafyx_mode) bits */
#define G_ENABLE    1
#define G_UL_NOTEXT 2   /* Micro Labs only */
#define G_RS_WAIT   2   /* Radio Shack only */
#define G_XDEC      4
#define G_YDEC      8
#define G_XNOCLKR   16
#define G_YNOCLKR   32
#define G_XNOCLKW   64
#define G_YNOCLKW   128

/* Port 0xFF (grafyx_m3_mode) bits */
#define G3_COORD    0x80
#define G3_ENABLE   0x40
#define G3_COMMAND  0x20
#define G3_YLOW(v)  (((v)&0x1e)>>1)

typedef struct image_size_type {
    unsigned int width;
    unsigned int height;
    unsigned int bytes_per_line;
} IMAGE_SIZE_TYPE;

IMAGE_SIZE_TYPE imageSize = {
  /*width, height*/    8*G_XSIZE, 2*G_YSIZE,  /* if scale_x=1, scale_y=2 */
  /*bytes_per_line*/   G_XSIZE,  /* if scale = 1 */
};

#define HRG_MEMSIZE (1024 * 12)        /* 12k * 8 bit graphics memory */
static unsigned char hrg_screen[HRG_MEMSIZE];
static int hrg_pixel_x[2][6+1];
static int hrg_pixel_y[12+1];
static int hrg_pixel_width[2][6];
static int hrg_pixel_height[12];
static int hrg_enable = 0;
static int hrg_addr = 0;
static void hrg_update_char(int position);

/*
 * Key event queueing routines
 */
#define KEY_QUEUE_SIZE (32)
static int key_queue[KEY_QUEUE_SIZE];
static int key_queue_head;
static int key_queue_entries;

/* dummy buffer for stat() call */
struct stat statbuf;

/* Option handling */
typedef struct trs_opt_struct {
  char *name;
  void (*handler)(char *,int, char *);
  int hasArg;
  int intArg;
  char *strArg;
} trs_opt;

static void trs_opt_scale(char *arg, int intarg, char *stringarg);
static void trs_opt_resize3(char *arg, int intarg, char *stringarg);
static void trs_opt_resize4(char *arg, int intarg, char *stringarg);
static void trs_opt_fullscreen(char *arg, int intarg, char *stringarg);
static void trs_opt_debug(char *arg, int intarg, char *stringarg);
static void trs_opt_model(char *arg, int intarg, char *stringarg);
static void trs_opt_charset1(char *arg, int intarg, char *stringarg);
static void trs_opt_charset3(char *arg, int intarg, char *stringarg);
static void trs_opt_charset4(char *arg, int intarg, char *stringarg);
static void trs_opt_printer(char *arg, int intarg, char *stringarg);
static void trs_opt_string(char *arg, int intarg, char *stringarg);
static void trs_opt_disk(char *arg, int intarg, char *stringarg);
static void trs_opt_hard(char *arg, int intarg, char *stringarg);
static void trs_opt_stringy(char *arg, int intarg, char *stringarg);
static void trs_opt_wafer(char *arg, int intarg, char *stringarg);
static void trs_opt_cass(char *arg, int intarg, char *stringarg);
static void trs_opt_diskset(char *arg, int intarg, char *stringarg);
static void trs_opt_keystretch(char *arg, int intarg, char *stringarg);
static void trs_opt_borderwidth(char *arg, int intarg, char *stringarg);
static void trs_opt_microlabs(char *arg, int intarg, char *stringarg);
static void trs_opt_led(char *arg, int intarg, char *stringarg);
static void trs_opt_doubler(char *arg, int intarg, char *stringarg);
static void trs_opt_sizemap(char *arg, int intarg, char *stringarg);
#ifdef __linux
static void trs_opt_stepmap(char *arg, int intarg, char *stringarg);
#endif
static void trs_opt_truedam(char *arg, int intarg, char *stringarg);
static void trs_opt_samplerate(char *arg, int intarg, char *stringarg);
static void trs_opt_switches(char *arg, int intarg, char *stringarg);
static void trs_opt_shiftbracket(char *arg, int intarg, char *stringarg);
static void trs_opt_keypadjoy(char *arg, int intarg, char *stringarg);
static void trs_opt_joysticknum(char *arg, int intarg, char *stringarg);
static void trs_opt_foreground(char *arg, int intarg, char *stringarg);
static void trs_opt_background(char *arg, int intarg, char *stringarg);
static void trs_opt_guiforeground(char *arg, int intarg, char *stringarg);
static void trs_opt_guibackground(char *arg, int intarg, char *stringarg);
static void trs_opt_emtsafe(char *arg, int intarg, char *stringarg);
static void trs_opt_turbo(char *arg, int intarg, char *stringarg);
static void trs_opt_turborate(char *arg, int intarg, char *stringarg);
static void trs_opt_mousepointer(char *arg, int intarg, char *stringarg);
static void trs_opt_joybuttonmap(char *arg, int intarg, char *stringarg);
static void trs_opt_joyaxismapped(char *arg, int intarg, char *stringarg);
static void trs_opt_huffman(char *arg, int intarg, char *stringarg);
static void trs_opt_hypermem(char *arg, int intarg, char *stringarg);
static void trs_opt_supermem(char *arg, int intarg, char *stringarg);
static void trs_opt_selector(char *arg, int intarg, char *stringarg);
static void trs_opt_le18(char *arg, int intarg, char *stringarg);
static void trs_opt_lower(char *arg, int intarg, char *stringarg);
static void trs_opt_sound(char *arg, int intarg, char *stringarg);
static void trs_opt_scanlines(char *arg, int intarg, char *stringarg);

trs_opt options[] = {
{"scale",trs_opt_scale,1,0,NULL},
{"resize3",trs_opt_resize3,0,1,NULL},
{"resize4",trs_opt_resize4,0,1,NULL},
{"noresize3",trs_opt_resize3,0,0,NULL},
{"noresize4",trs_opt_resize4,0,0,NULL},
{"fullscreen",trs_opt_fullscreen,0,1,NULL},
{"nofullscreen",trs_opt_fullscreen,0,0,NULL},
{"debug",trs_opt_debug,0,1,NULL},
{"model",trs_opt_model,1,0,NULL},
{"charset1",trs_opt_charset1,1,0,NULL},
{"charset3",trs_opt_charset3,1,0,NULL},
{"charset4",trs_opt_charset4,1,0,NULL},
{"romfile",trs_opt_string,1,0,romfile},
{"romfile3",trs_opt_string,1,0,romfile3},
{"romfile4p",trs_opt_string,1,0,romfile4p},
{"disk0",trs_opt_disk,1,0,NULL},
{"disk1",trs_opt_disk,1,1,NULL},
{"disk2",trs_opt_disk,1,2,NULL},
{"disk3",trs_opt_disk,1,3,NULL},
{"disk4",trs_opt_disk,1,4,NULL},
{"disk5",trs_opt_disk,1,5,NULL},
{"disk6",trs_opt_disk,1,6,NULL},
{"disk7",trs_opt_disk,1,7,NULL},
{"hard0",trs_opt_hard,1,0,NULL},
{"hard1",trs_opt_hard,1,1,NULL},
{"hard2",trs_opt_hard,1,2,NULL},
{"hard3",trs_opt_hard,1,3,NULL},
{"stringy",trs_opt_stringy,0,1,NULL},
{"nostringy",trs_opt_stringy,0,0,NULL},
{"wafer0",trs_opt_wafer,1,0,NULL},
{"wafer1",trs_opt_wafer,1,1,NULL},
{"wafer2",trs_opt_wafer,1,2,NULL},
{"wafer3",trs_opt_wafer,1,3,NULL},
{"wafer4",trs_opt_wafer,1,4,NULL},
{"wafer5",trs_opt_wafer,1,5,NULL},
{"wafer6",trs_opt_wafer,1,6,NULL},
{"wafer7",trs_opt_wafer,1,7,NULL},
{"cassette",trs_opt_cass,1,0,NULL},
{"diskset",trs_opt_diskset,1,0,NULL},
{"diskdir",trs_opt_string,1,0,trs_disk_dir},
{"harddir",trs_opt_string,1,0,trs_hard_dir},
{"cassdir",trs_opt_string,1,0,trs_cass_dir},
{"disksetdir",trs_opt_string,1,0,trs_disk_set_dir},
{"statedir",trs_opt_string,1,0,trs_state_dir},
{"printer",trs_opt_printer,1,0,NULL},
{"printerdir",trs_opt_string,1,0,trs_printer_dir},
{"printercmd",trs_opt_string,1,0,trs_printer_command},
{"keystretch",trs_opt_keystretch,1,0,NULL},
{"borderwidth",trs_opt_borderwidth,1,0,NULL},
{"microlabs",trs_opt_microlabs,0,1,NULL},
{"nomicrolabs",trs_opt_microlabs,0,0,NULL},
{"showled",trs_opt_led,0,1,NULL},
{"hideled",trs_opt_led,0,0,NULL},
{"doubler",trs_opt_doubler,1,0,NULL},
{"sizemap",trs_opt_sizemap,1,0,NULL},
#ifdef __linux
{"stepmap",trs_opt_stepmap,1,0,NULL},
#endif
{"truedam",trs_opt_truedam,0,1,NULL},
{"notruedam",trs_opt_truedam,0,0,NULL},
{"samplerate",trs_opt_samplerate,1,0,NULL},
{"serial",trs_opt_string,1,0,trs_uart_name},
{"switches",trs_opt_switches,1,0,NULL},
{"shiftbracket",trs_opt_shiftbracket,0,1,NULL},
{"noshiftbracket",trs_opt_shiftbracket,0,0,NULL},
{"keypadjoy",trs_opt_keypadjoy,0,1,NULL},
{"nokeypadjoy",trs_opt_keypadjoy,0,0,NULL},
{"joysticknum",trs_opt_joysticknum,1,0,NULL},
{"foreground",trs_opt_foreground,1,0,NULL},
{"background",trs_opt_background,1,0,NULL},
{"guiforeground",trs_opt_guiforeground,1,0,NULL},
{"guibackground",trs_opt_guibackground,1,0,NULL},
{"emtsafe",trs_opt_emtsafe,0,1,NULL},
{"noemtsafe",trs_opt_emtsafe,0,0,NULL},
{"turbo",trs_opt_turbo,0,1,NULL},
{"noturbo",trs_opt_turbo,0,0,NULL},
{"turborate",trs_opt_turborate,1,0,NULL},
{"mousepointer",trs_opt_mousepointer,0,1,NULL},
{"nomousepointer",trs_opt_mousepointer,0,0,NULL},
{"joybuttonmap",trs_opt_joybuttonmap,1,0,NULL},
{"joyaxismapped",trs_opt_joyaxismapped,0,1,NULL},
{"nojoyaxismapped",trs_opt_joyaxismapped,0,0,NULL},
{"huffman",trs_opt_huffman,0,1,NULL},
{"nohuffman",trs_opt_huffman,0,0,NULL},
{"hypermem",trs_opt_hypermem,0,1,NULL},
{"nohypermem",trs_opt_hypermem,0,0,NULL},
{"supermem",trs_opt_supermem,0,1,NULL},
{"nosupermem",trs_opt_supermem,0,0,NULL},
{"selector",trs_opt_selector,0,1,NULL},
{"noselector",trs_opt_selector,0,0,NULL},
{"le18",trs_opt_le18,0,1,NULL},
{"nole18",trs_opt_le18,0,0,NULL},
{"lower",trs_opt_lower,0,1,NULL},
{"nolower",trs_opt_lower,0,0,NULL},
{"sound",trs_opt_sound,0,1,NULL},
{"nosound",trs_opt_sound,0,0,NULL},
{"scanlines",trs_opt_scanlines,0,1,NULL},
{"noscanlines",trs_opt_scanlines,0,0,NULL},
};

static const int num_options = sizeof(options)/sizeof(trs_opt);

/* Private routines */
static void bitmap_init();
static void call_function(int function);
static char *charset_name(int charset);
static void trs_gui_save_gui(void);
static void trs_gui_rest_gui(void);

extern char *program_name;

static void stripWhitespace (char *inputStr)
{
  char *start, *end;

  start = inputStr;
  while (*start && (*start == ' '|| *start == '\t' || *start == '\r' || *start == '\n'))
    start++;
  memmove(inputStr, start, strlen(start) + 1);
  end = inputStr + strlen(inputStr) - 1;
  while (*end && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
    end--;
  *(end + 1) = '\0';
}

static char *charset_name(int charset)
{
  switch(charset) {
    case 0:
      return "early";
    case 1:
      return "stock";
    case 2:
      return "lcmod";
    case 3:
    default:
      return "wider";
    case 4:
    case 7:
      return "katakana";
    case 5:
    case 8:
      return "international";
    case 6:
    case 9:
      return "bold";
    case 10:
      return "genie";
    case 11:
      return "ht-1080z";
    case 12:
      return "videogenie";
  }
}

int trs_write_config_file(const char *filename)
{
  FILE *config_file;
  int i;

  if ((config_file = fopen(filename,"w")) == NULL)
    return -1;

  fprintf(config_file,"scale=%d\n",scale_x);
  fprintf(config_file,"%sresize3\n",resize3 ? "" : "no");
  fprintf(config_file,"%sresize4\n",resize4 ? "" : "no");
  fprintf(config_file,"model=");
  switch(trs_model) {
    case 1:
      fprintf(config_file,"1\n");
      break;
    case 3:
      fprintf(config_file,"3\n");
      break;
    case 4:
      fprintf(config_file,"4\n");
      break;
    case 5:
      fprintf(config_file,"4P\n");
      break;
  }
  fprintf(config_file,"charset1=%s\n",charset_name(trs_charset1));
  fprintf(config_file,"charset3=%s\n",charset_name(trs_charset3));
  fprintf(config_file,"charset4=%s\n",charset_name(trs_charset4));
  fprintf(config_file,"romfile=%s\n",romfile);
  fprintf(config_file,"romfile3=%s\n",romfile3);
  fprintf(config_file,"romfile4p=%s\n",romfile4p);
  fprintf(config_file,"diskdir=%s\n",trs_disk_dir);
  fprintf(config_file,"harddir=%s\n",trs_hard_dir);
  fprintf(config_file,"cassdir=%s\n",trs_cass_dir);
  fprintf(config_file,"disksetdir=%s\n",trs_disk_set_dir);
  fprintf(config_file,"statedir=%s\n",trs_state_dir);
  fprintf(config_file,"printer=%d\n",trs_printer);
  fprintf(config_file,"printerdir=%s\n",trs_printer_dir);
  fprintf(config_file,"printercmd=%s\n",trs_printer_command);
  fprintf(config_file,"keystretch=%d\n",stretch_amount);
  fprintf(config_file,"borderwidth=%d\n",window_border_width);
  fprintf(config_file,"%sfullscreen\n",fullscreen ? "" : "no");
  fprintf(config_file,"%smicrolabs\n",grafyx_microlabs ? "" : "no");
  fprintf(config_file,"%s\n",trs_show_led ? "showled" : "hideled");
  fprintf(config_file,"doubler=");
  switch(trs_disk_doubler) {
    case TRSDISK_PERCOM:
      fprintf(config_file,"percom\n");
      break;
    case TRSDISK_TANDY:
      fprintf(config_file,"tandy\n");
      break;
    case TRSDISK_BOTH:
      fprintf(config_file,"both\n");
      break;
    case TRSDISK_NODOUBLER:
      fprintf(config_file,"none\n");
      break;
  }
  fprintf(config_file, "sizemap=%d,%d,%d,%d,%d,%d,%d,%d\n",
          trs_disk_getsize(0),
          trs_disk_getsize(1),
          trs_disk_getsize(2),
          trs_disk_getsize(3),
          trs_disk_getsize(4),
          trs_disk_getsize(5),
          trs_disk_getsize(6),
          trs_disk_getsize(7));
#ifdef __linux
  fprintf(config_file, "stepmap=%d,%d,%d,%d,%d,%d,%d,%d\n",
          trs_disk_getstep(0),            /* Corrected to trs_disk_getstep vs getsize  */
          trs_disk_getstep(1),            /* Corrected by Larry Kraemer 08-01-2011 */
          trs_disk_getstep(2),
          trs_disk_getstep(3),
          trs_disk_getstep(4),
          trs_disk_getstep(5),
          trs_disk_getstep(6),
          trs_disk_getstep(7));
#endif
  fprintf(config_file,"%struedam\n",trs_disk_truedam ? "" : "no");
  fprintf(config_file,"samplerate=%d\n",cassette_default_sample_rate);
  fprintf(config_file,"serial=%s\n",trs_uart_name);
  fprintf(config_file,"switches=%d\n",trs_uart_switches);
  fprintf(config_file,"%sshiftbracket\n",trs_kb_bracket_state ? "" : "no");
  fprintf(config_file,"joysticknum=");
  if (trs_joystick_num == -1)
    fprintf(config_file,"none\n");
  else
    fprintf(config_file,"%d\n",trs_joystick_num);
  fprintf(config_file,"%skeypadjoy\n",trs_keypad_joystick ? "" : "no");
  fprintf(config_file,"foreground=0x%x\n",foreground);
  fprintf(config_file,"background=0x%x\n",background);
  fprintf(config_file,"guiforeground=0x%x\n",gui_foreground);
  fprintf(config_file,"guibackground=0x%x\n",gui_background);
  fprintf(config_file,"%sturbo\n",timer_overclock ? "" : "no");
  fprintf(config_file,"turborate=%d\n", timer_overclock_rate);

  for (i=0;i<8;i++) {
    const char *diskname = trs_disk_getfilename(i);

    if (diskname[0] != 0)
      fprintf(config_file,"disk%d=%s\n",i,diskname);
  }
  for (i=0;i<4;i++) {
    const char *diskname = trs_hard_getfilename(i);

    if (diskname[0] != 0)
      fprintf(config_file,"hard%d=%s\n",i,diskname);
  }
  for (i=0;i<8;i++) {
    const char *diskname = stringy_get_name(i);

    if (diskname[0] != 0)
      fprintf(config_file,"wafer%d=%s\n",i,diskname);
  }
  {
    const char *cassname = trs_cassette_getfilename();

    if (cassname[0] != 0)
      fprintf(config_file,"cassette=%s\n",cassname);
  }

  fprintf(config_file, "%smousepointer\n", mousepointer ? "" : "no");
  fprintf(config_file, "joybuttonmap=");
  for (i = 0; i < N_JOYBUTTONS; i++)
    fprintf(config_file, i < N_JOYBUTTONS - 1 ? "%d," : "%d\n", jbutton_map[i]);
  fprintf(config_file, "%sjoyaxismapped\n", jaxis_mapped ? "" : "no");

  fprintf(config_file, "%shuffman\n", huffman_ram ? "" : "no");
  fprintf(config_file, "%shypermem\n", hypermem ? "" : "no");
  fprintf(config_file, "%ssupermem\n", supermem ? "" : "no");
  fprintf(config_file, "%sselector\n", selector ? "" : "no");

  fprintf(config_file, "%sle18\n", lowe_le18 ? "" : "no");
  fprintf(config_file, "%slower\n", lowercase ? "" : "no");
  fprintf(config_file, "%ssound\n", trs_sound ? "" : "no");
  fprintf(config_file, "%sstringy\n", stringy ? "" : "no");
  fprintf(config_file, "%sscanlines\n", scanlines ? "" : "no");

  fclose(config_file);
  return 0;
}

void trs_set_to_defaults(void)
{
  int i;

  for (i=0;i<8;i++)
    trs_disk_remove(i);
  for (i=0;i<4;i++)
    trs_hard_remove(i);
  for (i=0;i<8;i++)
    stringy_remove(i);
  trs_cassette_remove();

  scale_x = 1;
  scale_y = 2;
  resize3 = 1;
  resize4 = 0;
  fullscreen = 0;
  trs_model = 1;
  lowercase = 1;
  trs_charset = 3;
  trs_charset1 = 3;
  trs_charset3 = 4;
  trs_charset4 = 8;
  strcpy(romfile,"level2.rom");
  strcpy(romfile3,"model3.rom");
  strcpy(romfile4p,"model4p.rom");
  trs_disk_dir[0] = 0;
  trs_hard_dir[0] = 0;
  trs_cass_dir[0] = 0;
  trs_disk_set_dir[0] = 0;
  trs_state_dir[0] = 0;
  trs_printer_dir[0] = 0;
#ifdef _WIN32
  strcpy(trs_printer_command,"notepad %s");
#else
  strcpy(trs_printer_command,"lpr %s");
#endif
  stretch_amount = STRETCH_AMOUNT;
  window_border_width = 2;
  grafyx_set_microlabs(FALSE);
  trs_show_led = TRUE;
  trs_disk_doubler = TRSDISK_BOTH;
  disksizes[0] = 5;            /* Disk Sizes are 5" or 8" for all Eight Default Drives */
  disksizes[1] = 5;            /* Corrected by Larry Kraemer 08-01-2011 */
  disksizes[2] = 5;
  disksizes[3] = 5;
  disksizes[4] = 8;
  disksizes[5] = 8;
  disksizes[6] = 8;
  disksizes[7] = 8;
  disksteps[0] = 1;            /* Disk Steps are 1 for Single Step, 2 for Double Step for all Eight Default Drives */
  disksteps[1] = 1;            /* Corrected by Larry Kraemer 08-01-2011 */
  disksteps[2] = 1;
  disksteps[3] = 1;
  disksteps[4] = 2;
  disksteps[5] = 2;
  disksteps[6] = 2;
  disksteps[7] = 2;
  trs_disk_truedam = 0;
  cassette_default_sample_rate = DEFAULT_SAMPLE_RATE;
  trs_uart_switches = 0x7 | TRS_UART_NOPAR | TRS_UART_WORD8;
  trs_kb_bracket(FALSE);
  trs_keypad_joystick = TRUE;
  trs_joystick_num = 0;
  foreground = WHITE;
  background = BLACK;
  gui_foreground = WHITE;
  gui_background = GREEN;
  trs_emtsafe = 1;
}

static void trs_opt_scale(char *arg, int intarg, char *stringarg)
{
  scale_x = atoi(arg);
  if (scale_x <= 0) scale_x = 1;
  if (scale_x > MAX_SCALE) scale_x = MAX_SCALE;
  scale_y = scale_x * 2;
}

static void trs_opt_resize3(char *arg, int intarg, char *stringarg)
{
  resize3 = intarg;
}

static void trs_opt_resize4(char *arg, int intarg, char *stringarg)
{
  resize4 = intarg;
}

static void trs_opt_fullscreen(char *arg, int intarg, char *stringarg)
{
  fullscreen = intarg;
}

static void trs_opt_debug(char *arg, int intarg, char *stringarg)
{
  debugger = intarg;
}

static void trs_opt_model(char *arg, int intarg, char *stringarg)
{
  if (strcmp(arg, "1") == 0 ||
      strcasecmp(arg, "I") == 0) {
    trs_model = 1;
  } else if (strcmp(arg, "3") == 0 ||
             strcasecmp(arg, "III") == 0) {
    trs_model = 3;
  } else if (strcmp(arg, "4") == 0 ||
             strcasecmp(arg, "IV") == 0) {
    trs_model = 4;
  } else if (strcasecmp(arg, "4P") == 0 ||
             strcasecmp(arg, "IVp") == 0) {
    trs_model = 5;
  } else
    error("TRS-80 Model %s not supported", arg);
}

static void trs_opt_charset1(char *arg, int intarg, char *stringarg)
{
  if (isdigit((int)*arg)) {
    trs_charset1 = atoi(arg);
    if (trs_charset1 < 0 || (trs_charset1 > 3 && (trs_charset1 < 10 || trs_charset1 > 12)))
      trs_charset1 = 3;
  } else
    switch ((int)tolower(*arg)) {
      case 'e': /*early*/
        trs_charset1 = 0;
        break;
      case 's': /*stock*/
        trs_charset1 = 1;
        break;
      case 'l': /*lcmod*/
        trs_charset1 = 2;
        break;
      case 'w': /*wider*/
        trs_charset1 = 3;
        break;
      case 'g': /*genie or german*/
        trs_charset1 = 10;
        break;
      case 'h': /*ht-1080z*/
        trs_charset1 = 11;
        break;
      case 'v': /*video genie*/
        trs_charset1 = 12;
        break;
      default:
        error("unknown charset1 name: %s", arg);
  }
}

static void trs_opt_charset3(char *arg, int intarg, char *stringarg)
{
  if (isdigit((int)*arg)) {
    trs_charset3 = atoi(arg);
    if (trs_charset3 < 4 || trs_charset3 > 6)
      trs_charset3 = 4;
  } else
    switch ((int)tolower(*arg)) {
      case 'k': /*katakana*/
        trs_charset3 = 4;
        break;
      case 'i': /*international*/
        trs_charset3 = 5;
        break;
      case 'b': /*bold*/
        trs_charset3 = 6;
        break;
      default:
        error("unknown charset3 name: %s", arg);
  }
}

static void trs_opt_charset4(char *arg, int intarg, char *stringarg)
{
  if (isdigit((int)*arg)) {
    trs_charset4 = atoi(arg);
    if (trs_charset4 < 7 || trs_charset4 > 9)
      trs_charset4 = 8;
  } else
    switch ((int)tolower(*arg)) {
      case 'k': /*katakana*/
        trs_charset4 = 7;
        break;
      case 'i': /*international*/
        trs_charset4 = 8;
        break;
      case 'b': /*bold*/
        trs_charset4 = 9;
        break;
      default:
        error("unknown charset4 name: %s", arg);
  }
}

static void trs_opt_printer(char *arg, int intarg, char *stringarg)
{
  if (isdigit((int)*arg)) {
    trs_printer = atoi(arg);
    if (trs_printer < 0 || trs_printer > 1)
      trs_printer = 0;
  } else
    switch ((int)tolower(*arg)) {
      case 'n': /*none*/
        trs_printer = 0;
        break;
      case 't': /*text*/
        trs_printer = 1;
        break;
      default:
        error("unknown printer type: %s", arg);
    }
}

static void trs_opt_string(char *arg, int intarg, char *stringarg)
{
  strcpy(stringarg, arg);
}

static void trs_opt_disk(char *arg, int intarg, char *stringarg)
{
  trs_disk_insert(intarg, arg);
}

static void trs_opt_hard(char *arg, int intarg, char *stringarg)
{
  trs_hard_attach(intarg, arg);
}

static void trs_opt_stringy(char *arg, int intarg, char *stringarg)
{
  stringy = intarg;
}

static void trs_opt_wafer(char *arg, int intarg, char *stringarg)
{
  stringy_insert(intarg, arg);
}

static void trs_opt_cass(char *arg, int intarg, char *stringarg)
{
  trs_cassette_insert(arg);
}

static void trs_opt_diskset(char *arg, int intarg, char *stringarg)
{
  if (trs_diskset_load(arg) == -1)
    error("Failed to load Diskset %s: %s", arg, strerror(errno));
}

static void trs_opt_keystretch(char *arg, int intarg, char *stringarg)
{
  stretch_amount = strtol(arg, NULL, 0);
}

static void trs_opt_borderwidth(char *arg, int intarg, char *stringarg)
{
  window_border_width = strtol(arg, NULL, 0);
}

static void trs_opt_microlabs(char *arg, int intarg, char *stringarg)
{
  grafyx_set_microlabs(intarg);
}

static void trs_opt_led(char *arg, int intarg, char *stringarg)
{
  trs_show_led = intarg;
}

static void trs_opt_doubler(char *arg, int intarg, char *stringarg)
{
  switch ((int)tolower(*arg)) {
    case 'p':
      trs_disk_doubler = TRSDISK_PERCOM;
      break;
    case 'r':
    case 't':
      trs_disk_doubler = TRSDISK_TANDY;
      break;
    case 'b':
    default:
      trs_disk_doubler = TRSDISK_BOTH;
      break;
    case 'n':
      trs_disk_doubler = TRSDISK_NODOUBLER;
      break;
    }
}

static void trs_opt_sizemap(char *arg, int intarg, char *stringarg)
{
  sscanf(arg, "%d,%d,%d,%d,%d,%d,%d,%d",
         &disksizes[0], &disksizes[1], &disksizes[2], &disksizes[3],
         &disksizes[4], &disksizes[5], &disksizes[6], &disksizes[7]);
}

#ifdef __linux
static void trs_opt_stepmap(char *arg, int intarg, char *stringarg)
{
  sscanf(arg, "%d,%d,%d,%d,%d,%d,%d,%d",
         &disksteps[0], &disksteps[1], &disksteps[2], &disksteps[3],
         &disksteps[4], &disksteps[5], &disksteps[6], &disksteps[7]);
}
#endif

static void trs_opt_truedam(char *arg, int intarg, char *stringarg)
{
  trs_disk_truedam = intarg;
}

static void trs_opt_samplerate(char *arg, int intarg, char *stringarg)
{
  cassette_default_sample_rate = strtol(arg, NULL, 0);
}

static void trs_opt_switches(char *arg, int intarg, char *stringarg)
{
  trs_uart_switches = strtol(arg, NULL, 0);
}

static void trs_opt_shiftbracket(char *arg, int intarg, char *stringarg)
{
  trs_kb_bracket(intarg);
}

static void trs_opt_keypadjoy(char *arg, int intarg, char *stringarg)
{
  trs_keypad_joystick = intarg;
}

static void trs_opt_joysticknum(char *arg, int intarg, char *stringarg)
{
  if (strcasecmp(arg,"none") == 0)
    trs_joystick_num = -1;
  else
    trs_joystick_num = atoi(arg);
}

static void trs_opt_foreground(char *arg, int intarg, char *stringarg)
{
  foreground = strtol(arg, NULL, 16);
}

static void trs_opt_background(char *arg, int intarg, char *stringarg)
{
  background = strtol(arg, NULL, 16);
}

static void trs_opt_guiforeground(char *arg, int intarg, char *stringarg)
{
  gui_foreground = strtol(arg, NULL, 16);
}

static void trs_opt_guibackground(char *arg, int intarg, char *stringarg)
{
  gui_background = strtol(arg, NULL, 16);
}

static void trs_opt_emtsafe(char *arg, int intarg, char *stringarg)
{
  trs_emtsafe = intarg;
}

static void trs_opt_turbo(char *arg, int intarg, char *stringarg)
{
  timer_overclock = intarg;
}

static void trs_opt_turborate(char *arg, int intarg, char *stringarg)
{
  timer_overclock_rate = atoi(arg);
  if (timer_overclock_rate <= 0)
    timer_overclock_rate = 1;
}

static void trs_opt_mousepointer(char *arg, int intarg, char *stringarg)
{
  mousepointer = intarg;
}

static void trs_opt_joybuttonmap(char *arg, int intarg, char *stringarg)
{
  unsigned int i;

  for (i = 0; i < N_JOYBUTTONS; i++) {
    char *ptr = strchr(arg, ',');

    if (ptr != NULL)
      *ptr = '\0';
    if (sscanf(arg, "%d", &jbutton_map[i]) == 0)
      jbutton_map[i] = -1;
    if (ptr != NULL)
      arg = ptr + 1;
  }
}

static void trs_opt_joyaxismapped(char *arg, int intarg, char *stringarg)
{
  jaxis_mapped = intarg;
}

static void trs_opt_huffman(char *arg, int intarg, char *stringarg)
{
  huffman_ram = intarg;
  if (huffman_ram)
    hypermem = 0;
}

static void trs_opt_hypermem(char *arg, int intarg, char *stringarg)
{
  hypermem = intarg;
  if (hypermem)
    huffman_ram = 0;
}

static void trs_opt_supermem(char *arg, int intarg, char *stringarg)
{
  supermem = intarg;
  if (supermem)
    selector = 0;
}

static void trs_opt_selector(char *arg, int intarg, char *stringarg)
{
  selector = intarg;
  if (selector)
    supermem = 0;
}

static void trs_opt_le18(char *arg, int intarg, char *stringarg)
{
  lowe_le18 = intarg;
}

static void trs_opt_lower(char *arg, int intarg, char *stringarg)
{
  lowercase = intarg;
}

static void trs_opt_sound(char *arg, int intarg, char *stringarg)
{
  trs_sound = intarg;
}

static void trs_opt_scanlines(char *arg, int intarg, char *stringarg)
{
  scanlines = intarg;
}

int trs_load_config_file()
{
  char line[FILENAME_MAX];
  char *arg;
  FILE *config_file;
  int i;

  trs_set_to_defaults();
  trs_disk_setsizes();
  trs_disk_setsteps();

  if (trs_config_file[0] == 0) {
#ifdef _WIN32
    snprintf(trs_config_file, FILENAME_MAX-1, "./sdltrs.t8c");
#else
    const char *home = getenv("HOME");

    snprintf(trs_config_file, FILENAME_MAX-1, "%s/.sdltrs.t8c", home);
#endif
  }

  if ((config_file = fopen(trs_config_file,"r")) == NULL) {
    if (trs_write_config_file(trs_config_file) == -1)
      error("Failed to write %s: %s", trs_config_file, strerror(errno));
    return -1;
  }

  while (fgets(line, sizeof(line), config_file)) {
    arg = strchr(line, '=');
    if (arg != NULL) {
      *arg++ = '\0';
      stripWhitespace(arg);
    }

    stripWhitespace(line);

    for (i=0;i<num_options;i++) {
      if (strcasecmp(line, options[i].name) == 0) {
        if (options[i].hasArg) {
          if (arg)
            (*options[i].handler)(arg,options[i].intArg,options[i].strArg);
        } else
          (*options[i].handler)(NULL,options[i].intArg,options[i].strArg);
        break;
      }
    }
    if (i == num_options)
      error("unrecognized option %s", line);
  }

  fclose(config_file);
  return 0;
}

void trs_parse_command_line(int argc, char **argv, int *debug)
{
  int i,j;

  /* Check for config or state files on the command line */
  trs_config_file[0] = 0;
  init_state_file[0] = 0;
  trs_cmd_file[0] = 0;
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      for (j=0;j<num_options;j++) {
        if (strcasecmp(&argv[i][1], options[j].name) == 0) {
          if (options[j].hasArg)
            i++;
          break;
        }
      }
    }
    else if (strlen(argv[i]) < 4) {
    }
    else if (strcasecmp(&argv[i][strlen(argv[i])-4],".t8c") == 0)
      snprintf(trs_config_file,FILENAME_MAX - 1,"%s",argv[i]);
    else if (strcasecmp(&argv[i][strlen(argv[i])-4],".t8s") == 0)
      snprintf(init_state_file,FILENAME_MAX - 1,"%s",argv[i]);
    else if (strcasecmp(&argv[i][strlen(argv[i])-4],".cmd") == 0)
      snprintf(trs_cmd_file,FILENAME_MAX - 1,"%s",argv[i]);
  }

  if (trs_load_config_file() == -1)
    error("Failed to load %s: %s", trs_config_file, strerror(errno));

  for (i = 1; i < argc; i++) {
    int argAvail = ((i + 1) < argc); /* is argument available? */

    for (j=0;j<num_options;j++) {
      if (argv[i][0] == '-') {
        if (strcasecmp(&argv[i][1], options[j].name) == 0) {
          if (options[j].hasArg) {
            if (argAvail)
              (*options[j].handler)(argv[++i],options[j].intArg,options[j].strArg);
          } else
            (*options[j].handler)(NULL,options[j].intArg,options[j].strArg);
          break;
        }
      }
    }
    if (j == num_options && argv[i][0] == '-')
      error("unrecognized option %s", argv[i]);
  }
  *debug = debugger;
  trs_disk_setsizes();
  trs_disk_setsteps();
}

void trs_disk_setsizes(void)
{
  unsigned int j;

  for (j=0; j<=7; j++) {
    if (disksizes[j] == 5 || disksizes[j] == 8)
      trs_disk_setsize(j, disksizes[j]);
    else
      error("bad value %d for disk %d size", disksizes[j], j);
  }
}

void trs_disk_setsteps(void)
{            /* Disk Steps are 1 for Single Step or 2 for Double Step for all Eight Default Drives */
  unsigned int j;

  for (j=0; j<=7; j++) {
    if (disksteps[j] == 1 || disksteps[j] == 2)
      trs_disk_setstep(j, disksteps[j]);
    else
      error("bad value %d for disk %d single/double step", disksteps[j], j);
  }
}

void trs_flip_fullscreen(void)
{
  static unsigned int window_scale_x = 1;
  static unsigned int window_scale_y = 2;

  fullscreen = !fullscreen;
  if (fullscreen) {
    window_scale_x = scale_x;
    window_scale_y = scale_y;
    if (scale_x != 1) {
      scale_x = 1;
      scale_y = 2;
    }
  }
  else {
    if (window_scale_x != 1) {
      scale_x = window_scale_x;
      scale_y = window_scale_y;
    }
  }

  trs_screen_init();
}

void trs_rom_init(void)
{
  switch(trs_model) {
    case 1:
      if (romfile[0] != 0) {
        if (trs_load_rom(romfile) == 0)
          break;
      }
      if (trs_rom1_size > 0)
        trs_load_compiled_rom(trs_rom1_size, trs_rom1);
      break;
    case 3:
    case 4:
      if (romfile3[0] != 0){
        if (trs_load_rom(romfile3) == 0)
          break;
      }
      if (trs_rom3_size > 0)
        trs_load_compiled_rom(trs_rom3_size, trs_rom3);
      break;
    case 5:
      if (romfile4p[0] != 0){
        if (trs_load_rom(romfile4p) == 0)
          break;
      }
      if (trs_rom4p_size > 0)
        trs_load_compiled_rom(trs_rom4p_size, trs_rom4p);
      break;
  }

  if (trs_model == 1 && stringy) {
    int i;

    /* Load ROM for ESF and adjust size */
    for (i = 0; i < trs_romesf_size; ++i)
      mem_write_rom(0x3000 + i, trs_romesf[i]);
    trs_rom_size = 0x3780;
  }
}

void trs_screen_var_reset()
{
  text80x24 = 0;
  screen640x240 = 0;
  screen_chars = 1024;
  row_chars = 64;
  col_chars = 16;
}

void trs_screen_caption(int turbo)
{
  char title[80];

  if (cpu_panel) {
    snprintf(title, 79, "AF:%04X BC:%04X DE:%04X HL:%04X IX/IY:%04X/%04X PC/SP:%04X/%04X",
             REG_AF, REG_BC, REG_DE, REG_HL, REG_IX, REG_IY, REG_PC, REG_SP);
  }
  else if (trs_model == 5) {
    snprintf(title, 79, "TRS-80 Model 4P %s%s%s", turbo ? "Turbo " : "",
             trs_paused ? "PAUSED " : "", trs_sound ? "" : "(Mute)");
  }
  else {
    snprintf(title, 79, "TRS-80 Model %d %s%s%s",trs_model, turbo ? "Turbo " : "",
             trs_paused ? "PAUSED " : "", trs_sound ? "" : "(Mute)");
  }
#ifdef SDL2
  SDL_SetWindowTitle(window, title);
#else
  SDL_WM_SetCaption(title,NULL);
#endif
  trs_turbo_led(turbo);
}

void trs_screen_init(void)
{
  SDL_Color colors[2];

#if defined(SDL2) || !defined(NOX)
  copyStatus = COPY_IDLE;
#endif
  if (trs_model == 1) {
    trs_charset = trs_charset1;
    currentmode = NORMAL;
  }
  else if (trs_model == 3) {
    trs_charset = trs_charset3;
    currentmode = NORMAL;
  }
  else
    trs_charset = trs_charset4;

  if (trs_model == 4)
    resize = resize4;
  else
    resize = resize3;

  if (trs_model == 1) {
    if (trs_charset < 3)
      cur_char_width = 6 * scale_x;
    else
      cur_char_width = 8 * scale_x;
    cur_char_height = TRS_CHAR_HEIGHT * scale_y;
  } else {
    cur_char_width = TRS_CHAR_WIDTH * scale_x;
    if (screen640x240 || text80x24)
      cur_char_height = TRS_CHAR_HEIGHT4 * scale_y;
    else
      cur_char_height = TRS_CHAR_HEIGHT * scale_y;
  }

  imageSize.width = 8*G_XSIZE*scale_x;
  imageSize.height = 2*G_YSIZE * scale_y/2;
  imageSize.bytes_per_line = G_XSIZE * scale_x;

  if (fullscreen)
    border_width = 0;
  else
    border_width = window_border_width;

  if (trs_show_led)
    led_width = 8;
  else
    led_width = 0;

  if (trs_model >= 3  && !resize) {
    OrigWidth = cur_char_width * 80 + 2 * border_width;
    left_margin = cur_char_width * (80 - row_chars)/2 + border_width;
    OrigHeight = TRS_CHAR_HEIGHT4 * scale_y * 24 + 2 * border_width + led_width;
    top_margin = (TRS_CHAR_HEIGHT4 * scale_y * 24 -
                 cur_char_height * col_chars)/2 + border_width;
  } else {
    OrigWidth = cur_char_width * row_chars + 2 * border_width;
    left_margin = border_width;
    OrigHeight = cur_char_height * col_chars + 2 * border_width + led_width;
    top_margin = border_width;
  }
#ifdef SDL2
  if (window == NULL) {
    window = SDL_CreateWindow(program_name,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              OrigWidth, OrigHeight,
                              SDL_WINDOW_SHOWN);
  }
  SDL_RaiseWindow(window);
  SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
  SDL_SetWindowSize(window, OrigWidth, OrigHeight);
  screen = SDL_GetWindowSurface(window);
#else
  screen = SDL_SetVideoMode(OrigWidth, OrigHeight, 0, fullscreen ?
                            SDL_ANYFORMAT | SDL_FULLSCREEN : SDL_ANYFORMAT);
  SDL_WarpMouse(OrigWidth / 2, OrigHeight / 2);
#endif
  SDL_ShowCursor(mousepointer ? SDL_ENABLE : SDL_DISABLE);

  trs_screen_caption(trs_timer_is_turbo());

  light_red = SDL_MapRGB(screen->format, 0x40,0x00,0x00);
  bright_red = SDL_MapRGB(screen->format, 0xff,0x00,0x00);
  light_orange = SDL_MapRGB(screen->format, 0x40,0x28,0x00);
  bright_orange = SDL_MapRGB(screen->format, 0xff,0xa0,0x00);

  if (image)
    SDL_FreeSurface(image);
  memset(grafyx,0,(2*G_YSIZE*MAX_SCALE) * (G_XSIZE*MAX_SCALE));
  image = SDL_CreateRGBSurfaceFrom(grafyx, imageSize.width, imageSize.height, 1,
                                   imageSize.bytes_per_line, 1, 1, 1, 0);
  colors[0].r = (background >> 16) & 0xFF;
  colors[0].g = (background >> 8) & 0xFF;
  colors[0].b = (background) & 0xFF;
  colors[1].r = (foreground >> 16) & 0xFF;
  colors[1].g = (foreground >> 8) & 0xFF;
  colors[1].b = (foreground) & 0xFF;
#ifdef SDL2
  SDL_SetPaletteColors(image->format->palette, colors,0,2);
#else
  SDL_SetPalette(image,SDL_LOGPAL, colors,0,2);
#endif

  TrsBlitMap(image->format->palette, screen->format);

  bitmap_init(foreground, background);

  trs_disk_led(-1,0);
  trs_hard_led(-1,0);
  trs_turbo_led(trs_timer_is_turbo());

  grafyx_redraw();
  drawnRectCount = MAX_RECTS; /* Will force redraw of whole screen */
}

void addToDrawList(SDL_Rect *rect)
{
  if (drawnRectCount < MAX_RECTS)
    drawnRects[drawnRectCount++] = *rect;
}

void DrawSelectionRectangle(int orig_x, int orig_y, int copy_x, int copy_y)
{
  int i,y;

  if (copy_x < orig_x) {
    int swap_x;

    swap_x = copy_x;
    copy_x = orig_x;
    orig_x = swap_x;
  }
  if (copy_y < orig_y) {
    int swap_y;

    swap_y = copy_y;
    copy_y = orig_y;
    orig_y = swap_y;
  }

  if (screen->format->BitsPerPixel == 8) {
    const int pitch = screen->pitch;
    Uint8 *start8;

    SDL_LockSurface(screen);

    for (y=orig_y;y < orig_y+scale_x;y++) {
      start8 = (Uint8 *) screen->pixels +
        (y * pitch) + orig_x;
      for (i=0;i<(copy_x-orig_x+scale_x);i++,start8++)
        *start8 ^= 0xFF;
    }
    if (copy_y > orig_y) {
      for (y=copy_y;y < copy_y+scale_x;y++) {
        start8 = (Uint8 *) screen->pixels +
          (y * pitch) + orig_x;
        for (i=0;i<(copy_x-orig_x+scale_x);i++,start8++)
          *start8 ^= 0xFF;
      }
    }
    for (y=orig_y+scale_x;y < copy_y;y++) {
      start8 = (Uint8 *) screen->pixels +
        (y * pitch) + orig_x;
      for (i=0;i<scale_x;i++)
        *start8++ ^= 0xFF;
    }
    if (copy_x > orig_x) {
      for (y=orig_y+scale_x;y < copy_y;y++) {
        start8 = (Uint8 *) screen->pixels +
          (y * pitch) + copy_x;
        for (i=0;i<scale_x;i++)
          *start8++ ^= 0xFF;
      }
    }
    SDL_UnlockSurface(screen);
  }
  else if (screen->format->BitsPerPixel == 16) {
    const int pitch2 = screen->pitch / 2;
    Uint16 *start16;

    SDL_LockSurface(screen);

    for (y=orig_y;y < orig_y+scale_x;y++) {
      start16 = (Uint16 *) screen->pixels +
        (y * pitch2) + orig_x;
      for (i=0;i<(copy_x-orig_x+scale_x);i++,start16++)
        *start16 ^= 0xFFFF;
    }
    if (copy_y > orig_y) {
      for (y=copy_y;y < copy_y+scale_x;y++) {
        start16 = (Uint16 *) screen->pixels +
          (y * pitch2) + orig_x;
        for (i=0;i<(copy_x-orig_x+scale_x);i++,start16++)
          *start16 ^= 0xFFFF;
      }
    }
    for (y=orig_y+scale_x;y < copy_y;y++) {
      start16 = (Uint16 *) screen->pixels +
        (y * pitch2) + orig_x;
      for (i=0;i<scale_x;i++)
        *start16++ ^= 0xFFFF;
    }
    if (copy_x > orig_x) {
      for (y=orig_y+scale_x;y < copy_y;y++) {
        start16 = (Uint16 *) screen->pixels +
          (y * pitch2) + copy_x;
        for (i=0;i<scale_x;i++)
          *start16++ ^= 0xFFFF;
      }
    }
    SDL_UnlockSurface(screen);
  }
  else if (screen->format->BitsPerPixel == 32) {
    const int pitch4 = screen->pitch / 4;
    Uint32 *start32;

    SDL_LockSurface(screen);

    for (y=orig_y;y<orig_y+scale_x;y++) {
      start32 = (Uint32 *) screen->pixels +
        (y * pitch4) + orig_x;
      for (i=0;i<(copy_x-orig_x+scale_x);i++,start32++)
        *start32 ^= 0xFFFFFFFF;
    }
    if (copy_y > orig_y) {
      for (y=copy_y;y<copy_y+scale_x;y++) {
        start32 = (Uint32 *) screen->pixels +
          (y * pitch4) + orig_x;
        for (i=0;i<(copy_x-orig_x+scale_x);i++,start32++)
          *start32 ^= 0xFFFFFFFF;
      }
    }
    for (y=orig_y+scale_x;y < copy_y;y++) {
      start32 = (Uint32 *) screen->pixels +
        (y * pitch4) + orig_x;
      for (i=0;i<scale_x;i++)
        *start32++ ^= 0xFFFFFFFF;
    }
    if (copy_x > orig_x) {
      for (y=orig_y+scale_x;y < copy_y;y++) {
        start32 = (Uint32 *) screen->pixels +
          (y * pitch4) + copy_x;
        for (i=0;i<scale_x;i++)
          *start32++ ^= 0xFFFFFFFF;
      }
    }
    SDL_UnlockSurface(screen);
  }
}

#if defined(SDL2) || !defined(NOX)
void ProcessCopySelection(int selectAll)
{
  static int orig_x = 0;
  static int orig_y = 0;
  static int end_x = 0;
  static int end_y = 0;
  static int copy_x = 0;
  static int copy_y = 0;
  static Uint8 mouse = 0;

  if (selectAll) {
    if (copyStatus == COPY_STARTED)
      return;
    if (copyStatus == COPY_DEFINED || copyStatus == COPY_CLEAR)
      DrawSelectionRectangle(orig_x, orig_y, end_x, end_y);
    orig_x = 0;
    orig_y = 0;
    copy_x = end_x = screen->w - scale_x;
    copy_y = end_y = screen->h - scale_x;
    DrawSelectionRectangle(orig_x, orig_y, end_x, end_y);
    drawnRectCount = MAX_RECTS;
    copyStatus = COPY_DEFINED;
    selectionStartX = orig_x - left_margin;
    selectionStartY = orig_y - top_margin;
    selectionEndX = copy_x - left_margin;
    selectionEndY = copy_y - top_margin;
  } else {
    mouse = SDL_GetMouseState(&copy_x, &copy_y);
    if ((copyStatus == COPY_IDLE) &&
        ((mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) == 0)) {
      return;
    }
  }

  switch(copyStatus) {
    case COPY_IDLE:
      if (selectAll) {
        copyStatus = COPY_DEFINED;
        orig_x = 0;
        orig_y = 0;
        selectionStartX = orig_x - left_margin;
        selectionStartY = orig_y - top_margin;
        selectionEndX = copy_x - left_margin;
        selectionEndY = copy_y - top_margin;
        DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
        drawnRectCount = MAX_RECTS;
      }
      else if (mouse & SDL_BUTTON(SDL_BUTTON_LEFT) ) {
        copyStatus = COPY_STARTED;
        orig_x = copy_x;
        orig_y = copy_y;
        DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
        drawnRectCount = MAX_RECTS;
      }
      end_x = copy_x;
      end_y = copy_y;
      break;
    case COPY_STARTED:
      DrawSelectionRectangle(orig_x, orig_y, end_x, end_y);
      if (mouse & SDL_BUTTON(SDL_BUTTON_LEFT))
        DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
      drawnRectCount = MAX_RECTS;
      end_x = copy_x;
      end_y = copy_y;
      if ((mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) == 0) {
        if (orig_x == copy_x && orig_y == copy_y) {
          copyStatus = COPY_IDLE;
        } else {
          DrawSelectionRectangle(orig_x, orig_y, end_x, end_y);
          copyStatus = COPY_DEFINED;
          selectionStartX = orig_x - left_margin;
          selectionStartY = orig_y - top_margin;
          selectionEndX = copy_x - left_margin;
          selectionEndY = copy_y - top_margin;
        }
      }
      break;
    case COPY_DEFINED:
      if (mouse & (SDL_BUTTON(SDL_BUTTON_LEFT) | SDL_BUTTON(SDL_BUTTON_RIGHT))) {
        copyStatus = COPY_STARTED;
        DrawSelectionRectangle(orig_x, orig_y, end_x, end_y);
        orig_x = end_x = copy_x;
        orig_y = end_y = copy_y;
        DrawSelectionRectangle(orig_x, orig_y, copy_x, copy_y);
        drawnRectCount = MAX_RECTS;
      }
      break;
    case COPY_CLEAR:
      DrawSelectionRectangle(orig_x, orig_y, end_x, end_y);
      drawnRectCount = MAX_RECTS;
      copyStatus = COPY_IDLE;
  }
}

void trs_end_copy()
{
  copyStatus = COPY_CLEAR;
}

void trs_paste_started()
{
  paste_state = PASTE_GETNEXT;
}

void trs_select_all()
{
  requestSelectAll = TRUE;
}
#endif

/*
 * Flush output to X server
 */
inline void trs_x_flush()
{
#if defined(SDL2) || !defined(NOX)
  if (!trs_emu_mouse || copyStatus != COPY_OFF)
    ProcessCopySelection(requestSelectAll);
  requestSelectAll = FALSE;
#endif
  if (drawnRectCount == 0)
    return;

  if (scanlines) {
    SDL_Rect rect;
    int y;

    rect.x = 0;
    rect.w = OrigWidth;
    rect.h = scale_y / 2;

    for (y = 0; y < OrigHeight - (led_width / 2); y += scale_y) {
      rect.y = y;
      SDL_FillRect(screen, &rect, background);
    }
  }

  if (drawnRectCount == MAX_RECTS)
#ifdef SDL2
    SDL_UpdateWindowSurface(window);
#else
    SDL_UpdateRect(screen,0,0,0,0);
#endif
  else
#ifdef SDL2
    SDL_UpdateWindowSurfaceRects(window,drawnRects,drawnRectCount);
#else
    SDL_UpdateRects(screen,drawnRectCount,drawnRects);
#endif
  drawnRectCount = 0;
}

void trs_exit(int confirm)
{
  extern int trs_gui_exit_sdltrs();
  static int recursion = 0;
  unsigned int i, ch;

  if (recursion) return;
  recursion = 1;

  if (confirm != 0) {
    if (confirm == 2)
      trs_gui_save_gui();
    if (!trs_gui_exit_sdltrs()) {
      if (confirm == 2)
        trs_gui_rest_gui();
      else
        trs_screen_refresh();
      trs_x_flush();
      recursion = 0;
      return;
    }
  }

  /* SDL cleanup */
  for (i = 0; i < 6; i++)
    for (ch = 0; ch < MAXCHARS; ch++) {
      free(trs_char[i][ch]->pixels);
      SDL_FreeSurface(trs_char[i][ch]);
    }
  for (i = 0; i < 3; i++)
    for (ch = 0; ch < 64; ch++)
      SDL_FreeSurface(trs_box[i][ch]);
  SDL_FreeSurface(image);

#ifdef SDL2
  SDL_DestroyWindow(window);
#endif
  SDL_Quit(); /* Will free screen */

  exit(0);
}

#if defined(SDL2) || !defined(NOX)
char *trs_get_copy_data()
{
  static char copy_data[2500];
  char data;
  char *curr_data = copy_data;
  char *screen_ptr;
  int col, line;
  int start_col, end_col, start_line, end_line;

  if (grafyx_enable && !grafyx_overlay) {
    copy_data[0] = 0;
    return(copy_data);
  }

  if (selectionStartX < 0)
    selectionStartX = 0;
  if (selectionStartY < 0)
    selectionStartY = 0;

  if (selectionStartX % cur_char_width == 0)
    start_col = selectionStartX / cur_char_width;
  else
    start_col = selectionStartX / cur_char_width + 1;

  if (selectionEndX % cur_char_width == cur_char_width - 1)
    end_col = selectionEndX / cur_char_width;
  else
    end_col = selectionEndX / cur_char_width - 1;

  if (selectionStartY % cur_char_height == 0)
    start_line = selectionStartY / cur_char_height;
  else
    start_line = selectionStartY / cur_char_height + 1;

  if (selectionEndY % cur_char_height >= cur_char_height / 2)
    end_line = selectionEndY / cur_char_height;
  else
    end_line = selectionEndY / cur_char_height - 1;

  if (end_col >= row_chars)
    end_col = row_chars - 1;
  if (end_line >= col_chars)
    end_line = col_chars - 1;

  for (line = start_line; line <= end_line; line++) {
    screen_ptr = (char*) &trs_screen[line * row_chars + start_col];
    for (col = start_col; col <= end_col; col++, screen_ptr++) {
      data = *screen_ptr;
      if (data < 0x20)
        data += 0x40;
      if (data >= 0x20 && data <= 0x7e)
        *curr_data++ = data;
      else
        *curr_data++ = ' ';
    }
    if (line != end_line) {
#ifdef _WIN32
      *curr_data++ = 0xd;
#endif
      *curr_data++ = 0xa;
    }
  }
  *curr_data = 0;
  return copy_data;
}
#endif

void call_function(int function)
{
  if (function == PAUSE) {
    trs_paused = !trs_paused;
    trs_screen_caption(trs_timer_is_turbo());
    if (!trs_paused)
      trs_screen_refresh();
  }
  else if (function == RESET) {
    trs_reset(1);
    trs_disk_led(-1, 0);
    trs_hard_led(-1, 0);
    trs_turbo_led(trs_timer_is_turbo());
  }
  else if (function == EXIT)
    trs_exit(0);
  else {
#ifndef SDL2
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#endif
    trs_pause_audio(1);
#if defined(SDL2) || !defined(NOX)
    copyStatus = COPY_OFF;
#endif
    switch (function) {
    case GUI:
      trs_gui();
      break;
    case JOYGUI:
      trs_gui_joy_gui();
      break;
    case KEYBRD:
      trs_gui_get_virtual_key();
      break;
    case SAVE:
      trs_gui_save_state();
      break;
    case LOAD:
      trs_gui_load_state();
      break;
    case DISK:
      trs_gui_disk_management();
      break;
    case HARD:
      trs_gui_hard_management();
      break;
    case STRINGY:
      trs_gui_stringy_management();
      break;
    case TAPE:
      trs_gui_cassette_management();
      break;
    case WRITE:
      trs_gui_write_config();
      break;
    case READ:
      if (trs_gui_read_config() == 0)
        trs_screen_init();
      break;
    case EMULATOR:
      trs_gui_model();
      break;
    case INTERFACE:
      trs_gui_display_management();
      break;
    case OTHER:
      trs_gui_misc_management();
      break;
    case KEYS:
      trs_gui_keys_sdltrs();
      break;
    case EXEC:
      trs_gui_exec_cmd();
      break;
    case SAVE_BMP:
      trs_gui_save_bmp();
      break;
    }
    trs_pause_audio(0);
#if defined(SDL2) || !defined(NOX)
    copyStatus = COPY_IDLE;
#endif
#ifndef SDL2
    SDL_EnableKeyRepeat(0,0);
#endif
    trs_screen_refresh();
    trs_x_flush();
  }
}

/*
 * Get and process SDL event(s).
 *   If wait is true, process one event, blocking until one is available.
 *   If wait is false, process as many events as are available, returning
 *     when none are left.
 * Handle interrupt-driven uart input here too.
 */
void trs_get_event(int wait)
{
  SDL_Event event;
#ifdef SDL2
  SDL_Keysym keysym;
  int text_char = 0;

  SDL_StartTextInput();
#else
  SDL_keysym keysym;
#endif
  Uint32 keyup;

  if (trs_model > 1)
    (void)trs_uart_check_avail();

  trs_x_flush();

  if (cpu_panel)
    trs_screen_caption(trs_timer_is_turbo());

  do {
#if defined(SDL2) || !defined(NOX)
    if (paste_state != PASTE_IDLE) {
      static unsigned short paste_key_uni;

      if (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
          if (paste_state == PASTE_KEYUP) {
            trs_xlate_keysym(0x10000 | paste_key_uni);
          }
          paste_state = PASTE_IDLE;
          return;
        }
      }

      if (paste_state == PASTE_GETNEXT) {
        if (!PasteManagerGetChar(&paste_key_uni))
          paste_lastkey = TRUE;
        else
          paste_lastkey = FALSE;
        trs_xlate_keysym(paste_key_uni);
        paste_state = PASTE_KEYDOWN;
        return;
      } else if (paste_state == PASTE_KEYDOWN) {
        trs_xlate_keysym(0x10000 | paste_key_uni);
        paste_state = PASTE_KEYUP;
        return;
      } else if (paste_state == PASTE_KEYUP) {
        if (paste_lastkey)
          paste_state = PASTE_IDLE;
        else
          paste_state = PASTE_GETNEXT;
      }
    }
#endif

    if (wait) {
      SDL_WaitEvent(&event);
    } else {
      if (!SDL_PollEvent(&event)) return;
    }
    switch(event.type) {
      case SDL_QUIT:
        trs_exit(0);
        break;
#ifdef SDL2
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
#else
      case SDL_ACTIVEEVENT:
        if (event.active.state & SDL_APPACTIVE) {
          if (event.active.gain) {
#endif
#if XDEBUG
            debug("Active\n");
#endif
            trs_screen_refresh();
#if defined(SDL2) || !defined(NOX)
            copyStatus = COPY_IDLE;
#endif
          }
#ifndef SDL2
        }
#endif
        break;

      case SDL_KEYDOWN:
        keysym  = event.key.keysym;
#if XDEBUG
        debug("KeyDown: mod 0x%x, scancode 0x%x keycode 0x%x, unicode 0x%x\n",
            keysym.mod, keysym.scancode, keysym.sym, keysym.unicode);
#endif
#if defined(SDL2) || !defined(NOX)
        if (keysym.sym != SDLK_LALT) {
          if (copyStatus != COPY_IDLE) {
            copyStatus = COPY_CLEAR;
            trs_x_flush();
          }
        }
#endif

        switch (keysym.sym) {
          /* Trap some function keys here */
          case SDLK_F7:
            call_function(GUI);
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          case SDLK_F8:
            trs_exit(!(keysym.mod & KMOD_SHIFT));
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          case SDLK_F9:
            if (!fullscreen)
              trs_debug();
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          case SDLK_F10:
            if (keysym.mod & KMOD_SHIFT)
            {
              trs_reset(1);
              trs_disk_led(-1,0);
              trs_hard_led(-1,0);
              trs_turbo_led(trs_timer_is_turbo());
            }
            else
              trs_reset(0);
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          case SDLK_F11:
            if (keysym.mod & KMOD_SHIFT)
              call_function(SAVE_BMP);
            else
              call_function(KEYS);
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          case SDLK_F12:
            trs_screen_caption(trs_timer_switch_turbo());
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          case SDLK_PAUSE:
            call_function(PAUSE);
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
#ifndef SDL2
          case SDLK_PRINT:
#else
          case SDLK_PRINTSCREEN:
#endif
            call_function(SAVE_BMP);
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
            break;
          default:
            break;
        }
        /* Trap the alt keys here */
        if (keysym.mod & KMOD_LALT) {
          switch (keysym.sym) {
#if defined(SDL2) || !defined(NOX)
            case SDLK_c:
              PasteManagerStartCopy(trs_get_copy_data());
              copyStatus = COPY_IDLE;
              break;
            case SDLK_v:
              PasteManagerStartPaste();
              break;
            case SDLK_a:
              requestSelectAll = TRUE;
              break;
#endif
#ifdef _WIN32
            case SDLK_F4:
              trs_exit(1);
              break;
#endif
            case SDLK_DELETE:
              trs_reset(0);
              break;
            case SDLK_INSERT:
              cpu_panel = !cpu_panel;
              trs_screen_caption(trs_timer_is_turbo());
              break;
            case SDLK_RETURN:
              trs_flip_fullscreen();
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_HOME:
              fullscreen = 0;
              scale_x = 1;
              scale_y = 2;
              trs_screen_init();
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_PLUS:
            case SDLK_PAGEDOWN:
              fullscreen = 0;
              scale_x++;
              if (scale_x > MAX_SCALE)
                scale_x = 1;
              scale_y = scale_x * 2;
              trs_screen_init();
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_MINUS:
            case SDLK_PAGEUP:
              fullscreen = 0;
              scale_x--;
              if (scale_x < 1)
                scale_x = MAX_SCALE;
              scale_y = scale_x * 2;
              trs_screen_init();
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_PERIOD:
              mousepointer = !mousepointer;
              SDL_ShowCursor(mousepointer ? SDL_ENABLE : SDL_DISABLE);
              break;
            case SDLK_b:
              trs_show_led = !trs_show_led;
              trs_screen_init();
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_d:
            case SDLK_f:
              call_function(DISK);
              break;
            case SDLK_e:
              call_function(EMULATOR);
              break;
            case SDLK_g:
              call_function(STRINGY);
              break;
            case SDLK_h:
              call_function(HARD);
              break;
            case SDLK_i:
              call_function(INTERFACE);
              break;
            case SDLK_j:
              call_function(JOYGUI);
              break;
            case SDLK_k:
              call_function(KEYS);
              break;
            case SDLK_l:
              call_function(LOAD);
              trs_screen_init();
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_m:
              call_function(GUI);
              break;
            case SDLK_n:
              trs_screen_caption(trs_timer_switch_turbo());
              break;
            case SDLK_o:
              call_function(OTHER);
              break;
            case SDLK_p:
              call_function(PAUSE);
              break;
            case SDLK_q:
              trs_exit(1);
              break;
            case SDLK_r:
              call_function(READ);
              break;
            case SDLK_s:
              call_function(SAVE);
              break;
            case SDLK_t:
              call_function(TAPE);
              break;
            case SDLK_u:
              trs_sound = !trs_sound;
              trs_screen_caption(trs_timer_is_turbo());
              break;
            case SDLK_w:
              call_function(WRITE);
              break;
            case SDLK_x:
              call_function(EXEC);
              break;
            case SDLK_y:
              scanlines = !scanlines;
              trs_screen_refresh();
              trs_x_flush();
              break;
            case SDLK_z:
              if (!fullscreen)
                trs_debug();
              break;
            case SDLK_0:
            case SDLK_1:
            case SDLK_2:
            case SDLK_3:
            case SDLK_4:
            case SDLK_5:
            case SDLK_6:
            case SDLK_7:
              {
                char filename[FILENAME_MAX];

                if (keysym.mod & KMOD_SHIFT) {
                  trs_disk_remove(keysym.sym-SDLK_0);
                } else {
#ifndef SDL2
                  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
                      SDL_DEFAULT_REPEAT_INTERVAL);
#endif
                  if (trs_gui_file_browse(trs_disk_dir, filename, NULL,0,
                        " Floppy Disk Image ") != -1)
                    trs_disk_insert(keysym.sym-SDLK_0, filename);
#ifndef SDL2
                  SDL_EnableKeyRepeat(0,0);
#endif
                  trs_screen_refresh();
                  trs_x_flush();
                }
              }
              break;
            default:
              break;
          }
#ifndef SDL2
          keysym.unicode = 0;
#endif
          keysym.sym = 0;
          break;
        }

#ifdef SDL2
        if (event.key.repeat)
          break;
#endif
        /* Make Shift + CapsLock give lower case */
        if (((keysym.mod & (KMOD_CAPS|KMOD_LSHIFT))
              == (KMOD_CAPS|KMOD_LSHIFT) ||
              ((keysym.mod & (KMOD_CAPS|KMOD_RSHIFT))
                == (KMOD_CAPS|KMOD_RSHIFT)))
#ifdef SDL2
          && keysym.sym >= 'A' && keysym.sym <= 'Z')
            keysym.sym = (int) keysym.sym + 0x20;
#else
          && keysym.unicode >= 'A' && keysym.unicode <= 'Z')
            keysym.unicode = (int) keysym.unicode + 0x20;
#endif
        if (keysym.sym == SDLK_RSHIFT && trs_model == 1)
          keysym.sym = SDLK_LSHIFT;
        if (last_key[keysym.scancode] != 0)
          trs_xlate_keysym(0x10000 | last_key[keysym.scancode]);

        if (trs_model == 1) {
               if (keysym.sym == SDLK_F1) keysym.sym = 0x115;
          else if (keysym.sym == SDLK_F2) keysym.sym = 0x120;
          else if (keysym.sym == SDLK_F3) keysym.sym = 0x121;
          else if (keysym.sym == SDLK_F4) keysym.sym = 0x122;
        }

#ifdef SDL2
        if ((keysym.mod & KMOD_RALT) && keysym.sym == SDLK_q)
          keysym.sym = 0x11e;
        /* Convert arrow/control/function/shift keys */
        switch (keysym.sym) {
          case SDLK_UP:
            keysym.sym = 0x111;
            break;
          case SDLK_DOWN:
            keysym.sym = 0x112;
            break;
          case SDLK_RIGHT:
            keysym.sym = 0x113;
            break;
          case SDLK_LEFT:
            keysym.sym = 0x114;
            break;
          case SDLK_INSERT:
            keysym.sym = 0x115;
            break;
          case SDLK_HOME:
            keysym.sym = 0x116;
            break;
          case SDLK_END:
            keysym.sym = 0x117;
            break;
          case SDLK_PAGEUP:
            keysym.sym = 0x118;
            break;
          case SDLK_PAGEDOWN:
            keysym.sym = 0x119;
            break;
          case SDLK_SCROLLLOCK:
            keysym.sym = 0x11e;
            break;
          case SDLK_F1:
            keysym.sym = 0x11a;
            break;
          case SDLK_F2:
            keysym.sym = 0x11b;
            break;
          case SDLK_F3:
            keysym.sym = 0x11c;
            break;
          case SDLK_F4:
            keysym.sym = 0x11d;
            break;
          case SDLK_F5:
            keysym.sym = 0x11e;
            break;
          case SDLK_F6:
            keysym.sym = 0x11f;
            break;
          case SDLK_RSHIFT:
            keysym.sym = 0x12f;
            break;
          case SDLK_LSHIFT:
            keysym.sym = 0x130;
            break;
          case SDLK_LCTRL:
            keysym.sym = 0x132;
            break;
          default:
            break;
        }

        if (keysym.mod & KMOD_SHIFT) {
          if (keysym.sym >= 0x20 && keysym.sym <= 0xDF) {
            text_char = 1;
            break;
          }
        }
#else
        if (keysym.sym < 0x100 && keysym.unicode >= 0x20 && keysym.unicode <= 0xFF) {
          last_key[keysym.scancode] = keysym.unicode;
          trs_xlate_keysym(keysym.unicode);
        } else
#endif
        if (keysym.sym != 0) {
          last_key[keysym.scancode] = keysym.sym;
          trs_xlate_keysym(keysym.sym);
        }
        break;

      case SDL_KEYUP:
        keysym  = event.key.keysym;
#if XDEBUG
        debug("KeyUp: mod 0x%x, scancode 0x%x keycode 0x%x, unicode 0x%x\n",
            keysym.mod, keysym.scancode, keysym.sym, keysym.unicode);
#endif
        if (keysym.mod & KMOD_LALT)
          break;
        keyup = last_key[event.key.keysym.scancode];
        last_key[event.key.keysym.scancode] = 0;
        trs_xlate_keysym(0x10000 | keyup);
        keysym.sym = 0;
        break;

      case SDL_JOYAXISMOTION:
        if (jaxis_mapped == 1 && (event.jaxis.axis == 0 || event.jaxis.axis == 1)) {
          static int hor_value = 0, ver_value = 0, hor_key = 0, ver_key = 0;
          int value = 0, trigger_keyup = 0, trigger_keydown = 0;

          if (event.jaxis.axis == 0)
            value = hor_value;
          else
            value = ver_value;

          if (event.jaxis.value < -JOY_BOUNCE) {
            if (value == 1)
              trigger_keyup = 1;
            if (value != -1)
              trigger_keydown = 1;
            value = -1;
          }
          else if (event.jaxis.value > JOY_BOUNCE) {
            if (value == -1)
              trigger_keyup = 1;
            if (value != 1)
              trigger_keydown = 1;
            value = 1;
          }
          else if (abs(event.jaxis.value) < JOY_BOUNCE/8) {
            if (value != 0)
              trigger_keyup = 1;
            value = 0;
          }

          if (trigger_keyup) {
            if (event.jaxis.axis == 0)
              trs_xlate_keysym(0x10000 | hor_key);
            else
              trs_xlate_keysym(0x10000 | ver_key);
          }
          if (trigger_keydown) {
            if (event.jaxis.axis == 0) {
              hor_key = (value == -1 ? SDLK_LEFT : SDLK_RIGHT);
              trs_xlate_keysym(hor_key);
            }
            else {
              ver_key = (value == -1 ? SDLK_UP : SDLK_DOWN);
              trs_xlate_keysym(ver_key);
            }
          }

          if (event.jaxis.axis == 0)
            hor_value = value;
          else
            ver_value = value;
        }
        else
          trs_joy_axis(event.jaxis.axis, event.jaxis.value);
        break;

      case SDL_JOYHATMOTION:
        trs_joy_hat(event.jhat.value);
        break;

      case SDL_JOYBUTTONUP:
        if (event.jbutton.button < N_JOYBUTTONS) {
          int key = jbutton_map[event.jbutton.button];

          if (key >= 0)
            trs_xlate_keysym(0x10000 | key);
          else if (key == -1)
            trs_joy_button_up();
        }
        else
          trs_joy_button_up();
        break;

      case SDL_JOYBUTTONDOWN:
        if (event.jbutton.button < N_JOYBUTTONS) {
          int key = jbutton_map[event.jbutton.button];

          if (key >= 0)
            trs_xlate_keysym(key);
          else if (key == -1)
            trs_joy_button_down();
          else {
            call_function(key);
#ifndef SDL2
            keysym.unicode = 0;
#endif
            keysym.sym = 0;
          }
        }
        else
          trs_joy_button_down();
        break;

#ifdef SDL2
      case SDL_TEXTINPUT:
        if (text_char) {
          text_char = event.text.text[0];
          trs_xlate_keysym(text_char);
          last_key[keysym.scancode] = text_char;
          text_char = 0;
        }
        break;
#endif

      default:
#if XDEBUG
      /* debug("Unhandled event: type %d\n", event.type); */
#endif
        break;
    }
    if (trs_paused) {
      if (fullscreen)
        trs_gui_display_pause();
    }
  } while (!wait);
#ifdef SDL2
  SDL_StopTextInput();
#endif
}

void trs_screen_expanded(int flag)
{
  int const bit = flag ? EXPANDED : 0;

  if ((currentmode ^ bit) & EXPANDED) {
    currentmode ^= EXPANDED;
    SDL_FillRect(screen,NULL,background);
    trs_screen_refresh();
  }
}

void trs_screen_inverse(int flag)
{
  int const bit = flag ? INVERSE : 0;
  int i;

  if ((currentmode ^ bit) & INVERSE) {
    currentmode ^= INVERSE;
    for (i = 0; i < screen_chars; i++) {
      if (trs_screen[i] & 0x80)
        trs_screen_write_char(i, trs_screen[i]);
    }
  }
}

void trs_screen_alternate(int flag)
{
  int const bit = flag ? ALTERNATE : 0;
  int i;

  if ((currentmode ^ bit) & ALTERNATE) {
    currentmode ^= ALTERNATE;
    for (i = 0; i < screen_chars; i++) {
      if (trs_screen[i] >= 0xc0)
        trs_screen_write_char(i, trs_screen[i]);
    }
  }
}

void trs_screen_640x240(int flag)
{
  if (flag == screen640x240) return;
  screen640x240 = flag;
  if (flag) {
    row_chars = 80;
    col_chars = 24;
    cur_char_height = TRS_CHAR_HEIGHT4 * scale_y;
  } else {
    row_chars = 64;
    col_chars = 16;
    cur_char_height = TRS_CHAR_HEIGHT * scale_y;
  }
  screen_chars = row_chars * col_chars;
  if (resize)
    trs_screen_init();
  else {
    left_margin = cur_char_width * (80 - row_chars)/2 + border_width;
    top_margin = (TRS_CHAR_HEIGHT4 * scale_y * 24 -
        cur_char_height * col_chars)/2 + border_width;
    if (left_margin > border_width || top_margin > border_width)
      SDL_FillRect(screen,NULL,background);
  }
  trs_screen_refresh();
}

void trs_screen_80x24(int flag)
{
  if (!grafyx_enable || grafyx_overlay)
    trs_screen_640x240(flag);
  text80x24 = flag;
}

void screen_init()
{
  unsigned int i;

  /* initially, screen is blank (i.e. full of spaces) */
  for (i = 0; i < sizeof(trs_screen); i++)
    trs_screen[i] = ' ';

  for (i = 0; i < 1024; i++) {
    trs_gui_screen[i] = ' ';
    trs_gui_screen_invert[i] = 0;
    trs_gui_screen_copy[i] = ' ';
    trs_gui_screen_invert_copy[i] = 0;
  }
}

void
boxes_init(int foreground, int background, int width, int height, int expanded)
{
  int graphics_char, bit;
  SDL_Rect fullrect;
  SDL_Rect bits[6];

  /*
   * Calculate what the 2x3 boxes look like.
   */
  bits[0].x = bits[2].x = bits[4].x = 0;
  bits[0].w = bits[2].w = bits[4].w =
    bits[1].x = bits[3].x = bits[5].x =  width / 2;
  bits[1].w = bits[3].w = bits[5].w = width - bits[1].x;

  bits[0].y = bits[1].y = 0;
  bits[0].h = bits[1].h =
    bits[2].y = bits[3].y = height / 3;
  bits[4].y = bits[5].y = (height * 2) / 3;
  bits[2].h = bits[3].h = bits[4].y - bits[2].y;
  bits[4].h = bits[5].h = height - bits[4].y;

  fullrect.x = 0;
  fullrect.y = 0;
  fullrect.h = height;
  fullrect.w = width;

  for (graphics_char = 0; graphics_char < 64; ++graphics_char) {
    if (trs_box[expanded][graphics_char])
      SDL_FreeSurface(trs_box[expanded][graphics_char]);
    trs_box[expanded][graphics_char] =
      SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
                           0x00ff0000, 0x0000ff00,0x000000ff,0);

    /* Clear everything */
    SDL_FillRect(trs_box[expanded][graphics_char], &fullrect, background);

    for (bit = 0 ; bit < 6; ++bit) {
      if (graphics_char & (1 << bit)) {
        SDL_FillRect(trs_box[expanded][graphics_char], &bits[bit], foreground);
      }
    }
  }
}

SDL_Surface *CreateSurfaceFromDataScale(char *data,
    unsigned int foreground,
    unsigned int background,
    unsigned int width,
    unsigned int height,
    unsigned int scale_x,
    unsigned int scale_y)
{
  static unsigned int *mydata, *currdata;
  static unsigned char *mypixels, *currpixel;
  unsigned int w;
  int i, j;

  /*
   * Allocate a bit more room than necessary - There shouldn't be
   * any proportional characters, but just in case...
   * These arrays never get released, but they are really not
   * too big, so we should be OK.
   */
  mydata = (unsigned int *)malloc(width * height *
      scale_x * scale_y * sizeof(unsigned int));
  mypixels= (unsigned char *)malloc(width * height * 8);
  if (mydata == NULL || mypixels == NULL)
    fatal("CreateSurfaceFromDataScale: failed to allocate memory");

  /* Read the character data */
  for (j= 0; (unsigned)j< width * height; j += 8)
    for (i= j + 7; i >= j; i--)
      *(mypixels + i)= (*(data + (j >> 3)) >> (i - j)) & 1;

  currdata = mydata;
  /* And prepare our rescaled character. */
  for (j= 0; (unsigned)j< height * scale_y; j++) {
    currpixel = mypixels + ((j/scale_y) * width);
    for (w= 0; w< width ; w++) {
      if (*currpixel++ == 0) {
        for (i=0;(unsigned)i<scale_x;i++)
          *currdata++ = background;
      }
      else {
        for (i=0;(unsigned)i<scale_x;i++)
          *currdata++ = foreground;
      }
    }
  }

  free(mypixels);

  return(SDL_CreateRGBSurfaceFrom(mydata, width*scale_x, height*scale_y, 32, width*scale_x*4,
         0x00ff0000, 0x0000ff00, 0x000000ff,0));
}

void bitmap_init(unsigned long foreground, unsigned long background)
{
  /* Initialize from built-in font bitmaps. */
  unsigned int i;

  for (i = 0; i < MAXCHARS; i++) {
    if (trs_char[0][i]) {
      free(trs_char[0][i]->pixels);
      SDL_FreeSurface(trs_char[0][i]);
    }
    trs_char[0][i] =
      CreateSurfaceFromDataScale(trs_char_data[trs_charset][i],
          foreground, background,
          TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
          scale_x,scale_y);
    if (trs_char[1][i]) {
      free(trs_char[1][i]->pixels);
      SDL_FreeSurface(trs_char[1][i]);
    }
    trs_char[1][i] =
      CreateSurfaceFromDataScale(trs_char_data[trs_charset][i],
          foreground, background,
          TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
          scale_x*2,scale_y);
    if (trs_char[2][i]) {
      free(trs_char[2][i]->pixels);
      SDL_FreeSurface(trs_char[2][i]);
    }
    trs_char[2][i] =
      CreateSurfaceFromDataScale(trs_char_data[trs_charset][i],
          background, foreground,
          TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
          scale_x,scale_y);
    if (trs_char[3][i]) {
      free(trs_char[3][i]->pixels);
      SDL_FreeSurface(trs_char[3][i]);
    }
    trs_char[3][i] =
      CreateSurfaceFromDataScale(trs_char_data[trs_charset][i],
          background, foreground,
          TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
          scale_x*2,scale_y);
    if (trs_char[4][i]) {
      free(trs_char[4][i]->pixels);
      SDL_FreeSurface(trs_char[4][i]);
    }
    /* For the GUI, make sure we have brackets, backslash and block graphics */
    if ((i>='[' && i<=']') || i>=128)
      trs_char[4][i] =
        CreateSurfaceFromDataScale(trs_char_data[0][i],
            gui_foreground, gui_background,
            TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
            scale_x,scale_y);
    else
      trs_char[4][i] =
        CreateSurfaceFromDataScale(trs_char_data[trs_charset][i],
            gui_foreground, gui_background,
            TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
            scale_x,scale_y);
    if (trs_char[5][i]) {
      free(trs_char[5][i]->pixels);
      SDL_FreeSurface(trs_char[5][i]);
    }
    if ((i>='[' && i<=']') || i>=128)
      trs_char[5][i] =
        CreateSurfaceFromDataScale(trs_char_data[0][i],
            gui_background, gui_foreground,
            TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
            scale_x,scale_y);
    else
      trs_char[5][i] =
        CreateSurfaceFromDataScale(trs_char_data[trs_charset][i],
            gui_background, gui_foreground,
            TRS_CHAR_WIDTH,TRS_CHAR_HEIGHT,
            scale_x,scale_y);
  }
  boxes_init(foreground, background,
      cur_char_width, TRS_CHAR_HEIGHT * scale_y, 0);
  boxes_init(foreground, background,
      cur_char_width*2, TRS_CHAR_HEIGHT * scale_y, 1);
  boxes_init(gui_foreground, gui_background,
      cur_char_width, TRS_CHAR_HEIGHT * scale_y, 2);
}

void trs_screen_refresh()
{
  int i, srcx, srcy, dunx, duny;
  SDL_Rect srcRect, destRect;

#if XDEBUG
  debug("trs_screen_refresh\n");
#endif
#if defined(SDL2)
  screen = SDL_GetWindowSurface(window);
#endif
  if (grafyx_enable && !grafyx_overlay) {
    srcx = cur_char_width * grafyx_xoffset;
    srcy = scale_y * grafyx_yoffset;
    srcRect.x = srcx;
    srcRect.y = srcy;
    srcRect.w = cur_char_width*row_chars;
    srcRect.h = cur_char_height*col_chars;
    destRect.x = left_margin;
    destRect.y = top_margin;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    SDL_BlitSurface(image, &srcRect, screen, &destRect);
    addToDrawList(&destRect);
    /* Draw wrapped portions if any */
    dunx = imageSize.width - srcx;
    if (dunx < cur_char_width*row_chars) {
      srcRect.x = 0;
      srcRect.y = srcy;
      srcRect.w = cur_char_width*row_chars - dunx;
      srcRect.h = cur_char_height*col_chars;
      destRect.x = left_margin + dunx;
      destRect.y = top_margin;
      destRect.w = srcRect.w;
      destRect.h = srcRect.h;
      SDL_BlitSurface(image, &srcRect, screen, &destRect);
      addToDrawList(&destRect);
    }
    duny = imageSize.height - srcy;
    if (duny < cur_char_height*col_chars) {
      srcRect.x = srcx;
      srcRect.y = 0;
      srcRect.w = cur_char_width*row_chars;
      srcRect.h = cur_char_height*col_chars - duny;
      destRect.x = left_margin;
      destRect.y = top_margin + duny;
      destRect.w = srcRect.w;
      destRect.h = srcRect.h;
      addToDrawList(&destRect);
      SDL_BlitSurface(image, &srcRect, screen, &destRect);
      if (dunx < cur_char_width*row_chars) {
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width*row_chars - dunx;
        srcRect.h = cur_char_height*col_chars - duny;
        destRect.x = left_margin + dunx;
        destRect.y = top_margin + duny;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        addToDrawList(&destRect);
        SDL_BlitSurface(image, &srcRect, screen, &destRect);
      }
    }
  } else {
    for (i = 0; i < screen_chars; i++)
      trs_screen_write_char(i, trs_screen[i]);
  }

  if (trs_show_led) {
    trs_disk_led(-1,0);
    trs_hard_led(-1,0);
    trs_turbo_led(trs_timer_is_turbo());
  }

  drawnRectCount = MAX_RECTS; /* Will force redraw of whole screen */
}

void trs_disk_led(int drive, int on_off)
{
  static int countdown[8] = {0,0,0,0,0,0,0,0};
  unsigned int i;
  SDL_Rect rect;

  if (trs_show_led) {
    int drive0_led_x = border_width;
    rect.w = 16*scale_x;
    rect.h = 2*scale_y;
    rect.y = OrigHeight - led_width/2;

    if (drive == -1) {
      for (i=0;i<8;i++) {
        rect.x = drive0_led_x + 24*scale_x*i;
        SDL_FillRect(screen, &rect, light_red);
        addToDrawList(&rect);
      }
    }
    else if (on_off) {
      if (countdown[drive] == 0) {
        rect.x = drive0_led_x + 24*scale_x*drive;
        SDL_FillRect(screen, &rect, bright_red);
        addToDrawList(&rect);
      }
      countdown[drive] = 2*timer_hz;
    }
    else {
      for (i=0;i<8;i++) {
        if (countdown[i]) {
          countdown[i]--;
          if (countdown[i] == 0) {
            rect.x = drive0_led_x + 24*scale_x*i;
            SDL_FillRect(screen, &rect, light_red);
            addToDrawList(&rect);
          }
        }
      }
    }
  }
}

void trs_hard_led(int drive, int on_off)
{
  static int countdown[4] = {0,0,0,0};
  unsigned int i;
  SDL_Rect rect;

  if (trs_show_led) {
    int drive0_led_x = OrigWidth - border_width - 88*scale_x;
    rect.w = 16*scale_x;
    rect.h = 2*scale_y;
    rect.y = OrigHeight - led_width/2;

    if (drive == -1) {
      for (i=0;i<4;i++) {
        rect.x = drive0_led_x + 24*scale_x*i;
        SDL_FillRect(screen, &rect, light_red);
        addToDrawList(&rect);
      }
    }
    else if (on_off) {
      if (countdown[drive] == 0) {
        rect.x = drive0_led_x + 24*scale_x*drive;
        SDL_FillRect(screen, &rect, bright_red);
        addToDrawList(&rect);
      }
      countdown[drive] = timer_hz/2;
    }
    else {
      for (i=0;i<4;i++) {
        if (countdown[i]) {
          countdown[i]--;
          if (countdown[i] == 0) {
            rect.x = drive0_led_x + 24*scale_x*i;
            SDL_FillRect(screen, &rect, light_red);
            addToDrawList(&rect);
          }
        }
      }
    }
  }
}

void trs_turbo_led(int turbo)
{
  if (trs_show_led) {
    SDL_Rect rect;

    rect.w = 16*scale_x;
    rect.h = 2*scale_y;
    rect.x = (OrigWidth - border_width) / 2 - 8 * scale_x;
    rect.y = OrigHeight - led_width / 2;

    if (turbo)
      SDL_FillRect(screen, &rect, bright_orange);
    else
      SDL_FillRect(screen, &rect, light_orange);
    addToDrawList(&rect);
  }
}

void trs_screen_write_char(int position, int char_index)
{
  int row,col,destx,desty;
  SDL_Rect srcRect, destRect;

  trs_screen[position] = char_index;
  if (position >= screen_chars)
    return;
  if ((currentmode & EXPANDED) && (position & 1))
    return;
  if (grafyx_enable && !grafyx_overlay)
    return;

  row = position / row_chars;
  col = position - (row * row_chars);
  destx = col * cur_char_width + left_margin;
  desty = row * cur_char_height + top_margin;

  if (trs_model == 1 && char_index >= 0xc0) {
    /* On Model I, 0xc0-0xff is another copy of 0x80-0xbf */
    char_index -= 0x40;
  }
  if (char_index >= 0x80 && char_index <= 0xbf && !(currentmode & INVERSE)) {
    /* Use graphics character bitmap instead of font */
    switch (currentmode & EXPANDED) {
      case NORMAL:
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width;
        srcRect.h = cur_char_height;
        destRect.x = destx;
        destRect.y = desty;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        SDL_BlitSurface(trs_box[0][char_index-0x80], &srcRect, screen, &destRect);
        addToDrawList(&destRect);
        break;
      case EXPANDED:
        /* use expanded graphics character bitmap instead of font */
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width*2;
        srcRect.h = cur_char_height;
        destRect.x = destx;
        destRect.y = desty;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        SDL_BlitSurface(trs_box[1][char_index-0x80], &srcRect, screen, &destRect);
        addToDrawList(&destRect);
        break;
    }
  } else {
    /* Draw character using a builtin bitmap */
    if (trs_model > 1 && char_index >= 0xc0 &&
        (currentmode & (ALTERNATE+INVERSE)) == 0) {
      char_index -= 0x40;
    }
    switch (currentmode & ~ALTERNATE) {
      case NORMAL:
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width;
        srcRect.h = cur_char_height;
        destRect.x = destx;
        destRect.y = desty;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        SDL_BlitSurface(trs_char[0][char_index], &srcRect, screen, &destRect);
        addToDrawList(&destRect);
        break;
      case EXPANDED:
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width*2;
        srcRect.h = cur_char_height;
        destRect.x = destx;
        destRect.y = desty;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        SDL_BlitSurface(trs_char[1][char_index], &srcRect, screen, &destRect);
        addToDrawList(&destRect);
        break;
      case INVERSE:
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width;
        srcRect.h = cur_char_height;
        destRect.x = destx;
        destRect.y = desty;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        if (char_index & 0x80)
          SDL_BlitSurface(trs_char[2][char_index & 0x7f], &srcRect, screen, &destRect);
        else
          SDL_BlitSurface(trs_char[0][char_index & 0x7f], &srcRect, screen, &destRect);
        addToDrawList(&destRect);
        break;
      case EXPANDED+INVERSE:
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = cur_char_width*2;
        srcRect.h = cur_char_height;
        destRect.x = destx;
        destRect.y = desty;
        destRect.w = srcRect.w;
        destRect.h = srcRect.h;
        if (char_index & 0x80)
          SDL_BlitSurface(trs_char[3][char_index & 0x7f], &srcRect, screen, &destRect);
        else
          SDL_BlitSurface(trs_char[1][char_index & 0x7f], &srcRect, screen, &destRect);
        addToDrawList(&destRect);
        break;
    }
  }
  if (grafyx_enable) {
    /* assert(grafyx_overlay); */
    int srcx, srcy, duny;

    srcx = ((col+grafyx_xoffset) % G_XSIZE)*cur_char_width;
    srcy = (row*cur_char_height + grafyx_yoffset*scale_y)
      % (G_YSIZE*scale_y);
    srcRect.x = srcx;
    srcRect.y = srcy;
    srcRect.w = cur_char_width;
    srcRect.h = cur_char_height;
    destRect.x = destx;
    destRect.y = desty;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    addToDrawList(&destRect);
    TrsSoftBlit(image, &srcRect, screen, &destRect,1);
    /* Draw wrapped portion if any */
    duny = imageSize.height - srcy;
    if (duny < cur_char_height) {
      srcRect.x = srcx;
      srcRect.y = 0;
      srcRect.w = cur_char_width;
      srcRect.h = cur_char_height - duny;
      destRect.x = destx;
      destRect.y = desty + duny;
      destRect.w = srcRect.w;
      destRect.h = srcRect.h;
      addToDrawList(&destRect);
      TrsSoftBlit(image, &srcRect, screen, &destRect,1);
    }
  }
  if (hrg_enable)
    hrg_update_char(position);
}

void trs_gui_refresh()
{
#if defined(SDL2)
  screen = SDL_GetWindowSurface(window);
#endif
  drawnRectCount = MAX_RECTS; /* Will force redraw of whole screen */
}

void trs_gui_write_char(int position, int char_index, int invert)
{
  int row,col,destx,desty;
  SDL_Rect srcRect, destRect;

  if (position >= screen_chars)
    return;

  trs_gui_screen[position] = char_index;
  trs_gui_screen_invert[position] = invert;

  /* Add offsets if we are in 80x24 mode */
  if (row_chars != 64) {
    row = position / 64;
    col = position - (row * 64);
    position = (row + (col_chars-16)/2) * row_chars +
      col + (row_chars-64)/2;
  }

  row = position / row_chars;
  col = position - (row * row_chars);
  destx = col * cur_char_width + left_margin;
  desty = row * cur_char_height + top_margin;

  if (trs_model == 1 && char_index >= 0xc0)
    /* On Model I, 0xc0-0xff is another copy of 0x80-0xbf */
    char_index -= 0x40;
  if (char_index >= 0x80 && char_index <= 0xbf && !(currentmode & INVERSE)) {
    /* Use graphics character bitmap instead of font */
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = cur_char_width;
    srcRect.h = cur_char_height;
    destRect.x = destx;
    destRect.y = desty;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    SDL_BlitSurface(trs_box[2][char_index-0x80], &srcRect, screen, &destRect);
    addToDrawList(&destRect);
  } else {
    /* Draw character using a builtin bitmap */
    if (trs_model > 1 && char_index >= 0xc0 &&
        (currentmode & (ALTERNATE+INVERSE)) == 0)
      char_index -= 0x40;
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = cur_char_width;
    srcRect.h = cur_char_height;
    destRect.x = destx;
    destRect.y = desty;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    if (invert)
      SDL_BlitSurface(trs_char[5][char_index], &srcRect, screen, &destRect);
    else
      SDL_BlitSurface(trs_char[4][char_index], &srcRect, screen, &destRect);
    addToDrawList(&destRect);
  }
}

void trs_gui_clear_screen(void)
{
  unsigned int i;

  for (i=0;i<1024;i++)
    trs_gui_write_char(i,' ',0);
}

void trs_gui_save_gui(void)
{
  unsigned int i;

  for (i=0;i<1024;i++) {
    trs_gui_screen_copy[i] = trs_gui_screen[i];
    trs_gui_screen_invert_copy[i] = trs_gui_screen_invert[i];
  }
}

void trs_gui_rest_gui(void)
{
  unsigned int i;

  for (i=0;i<1024;i++) {
    trs_gui_screen[i] = trs_gui_screen_copy[i];
    trs_gui_screen_invert[i] = trs_gui_screen_invert_copy[i];
    trs_gui_write_char(i, trs_gui_screen[i], trs_gui_screen_invert[i]);
  }
}

 /* Copy lines 1 through col_chars-1 to lines 0 through col_chars-2.
    Doesn't need to clear line col_chars-1. */
void trs_screen_scroll()
{
  int i = 0;
  SDL_Rect srcRect, destRect;

  for (i = row_chars; i < screen_chars; i++)
    trs_screen[i-row_chars] = trs_screen[i];

  if (grafyx_enable) {
    if (grafyx_overlay)
      trs_screen_refresh();
  } else if (hrg_enable)
    trs_screen_refresh();
  else {
    srcRect.x = left_margin;
    srcRect.y = cur_char_height+top_margin;
    srcRect.w = cur_char_width*row_chars;
    srcRect.h = cur_char_height*(col_chars-1);
    destRect.x = left_margin;
    destRect.y = top_margin;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    SDL_BlitSurface(screen, &srcRect, screen, &destRect);
    addToDrawList(&destRect);
  }
}

void grafyx_write_byte(int x, int y, char byte)
{
  char exp[MAX_SCALE];
  int i, j;
  int const screen_x = ((x - grafyx_xoffset + G_XSIZE) % G_XSIZE);
  int const screen_y = ((y - grafyx_yoffset + G_YSIZE) % G_YSIZE);
  int const on_screen = screen_x < row_chars &&
    screen_y < col_chars*cur_char_height/scale_y;
  SDL_Rect srcRect, destRect;

  if (grafyx_enable && grafyx_overlay && on_screen) {
    srcRect.x = x*cur_char_width;
    srcRect.y = y*scale_y;
    srcRect.w = cur_char_width;
    srcRect.h = scale_y;
    destRect.x = left_margin + screen_x*cur_char_width;
    destRect.y = top_margin + screen_y*scale_y;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    /* Erase old byte, preserving text */
    TrsSoftBlit(image, &srcRect, screen, &destRect,1);
  }

  /* Save new byte in local memory */
  grafyx_unscaled[y][x] = byte;
  switch (scale_x) {
    case 1:
    default:
      exp[0] = byte;
      break;
    case 2:
      exp[1] = ((byte & 0x01) + ((byte & 0x02) << 1)
             + ((byte & 0x04) << 2) + ((byte & 0x08) << 3)) * 3;
      exp[0] = (((byte & 0x10) >> 4) + ((byte & 0x20) >> 3)
             + ((byte & 0x40) >> 2) + ((byte & 0x80) >> 1)) * 3;
      break;
    case 3:
      exp[2] = ((byte & 0x01) + ((byte & 0x02) << 2)
             + ((byte & 0x04) << 4)) * 7;
      exp[1] = (((byte & 0x08) >> 2) + (byte & 0x10)
             + ((byte & 0x20) << 2)) * 7 + ((byte & 0x04) >> 2);
      exp[0] = (((byte & 0x40) >> 4) + ((byte & 0x80) >> 2)) * 7
             + ((byte & 0x20) >> 5) * 3;
      break;
    case 4:
      exp[3] = ((byte & 0x01) + ((byte & 0x02) << 3)) * 15;
      exp[2] = (((byte & 0x04) >> 2) + ((byte & 0x08) << 1)) * 15;
      exp[1] = (((byte & 0x10) >> 4) + ((byte & 0x20) >> 1)) * 15;
      exp[0] = (((byte & 0x40) >> 6) + ((byte & 0x80) >> 3)) * 15;
      break;
  }
  for (j=0; j<scale_y; j++)
    for (i=0; i<scale_x; i++)
      grafyx[(y*scale_y + j)*imageSize.bytes_per_line + x*scale_x + i] = exp[i];

  if (grafyx_enable && on_screen) {
    /* Draw new byte */
    srcRect.x = x*cur_char_width;
    srcRect.y = y*scale_y;
    srcRect.w = cur_char_width;
    srcRect.h = scale_y;
    destRect.x = left_margin + screen_x*cur_char_width;
    destRect.y = top_margin + screen_y*scale_y;
    destRect.w = srcRect.w;
    destRect.h = srcRect.h;
    addToDrawList(&destRect);
    if (grafyx_overlay)
      TrsSoftBlit(image, &srcRect, screen, &destRect,1);
    else
      TrsSoftBlit(image, &srcRect, screen, &destRect,0);
  }
}

void grafyx_redraw(void)
{
  char byte;
  char exp[MAX_SCALE];
  int i, j;
  int x, y;

  for (y=0;y<G_YSIZE;y++) {
    for (x=0;x<G_XSIZE;x++) {
      byte = grafyx_unscaled[y][x];
      switch (scale_x) {
        default:
        case 1:
          exp[0] = byte;
          break;
        case 2:
          exp[1] = ((byte & 0x01) + ((byte & 0x02) << 1)
                 + ((byte & 0x04) << 2) + ((byte & 0x08) << 3)) * 3;
          exp[0] = (((byte & 0x10) >> 4) + ((byte & 0x20) >> 3)
                 + ((byte & 0x40) >> 2) + ((byte & 0x80) >> 1)) * 3;
          break;
        case 3:
          exp[2] = ((byte & 0x01) + ((byte & 0x02) << 2)
                 + ((byte & 0x04) << 4)) * 7;
          exp[1] = (((byte & 0x08) >> 2) + (byte & 0x10)
                 + ((byte & 0x20) << 2)) * 7 + ((byte & 0x04) >> 2);
          exp[0] = (((byte & 0x40) >> 4) + ((byte & 0x80) >> 2)) * 7
                 + ((byte & 0x20) >> 5) * 3;
          break;
        case 4:
          exp[3] = ((byte & 0x01) + ((byte & 0x02) << 3)) * 15;
          exp[2] = (((byte & 0x04) >> 2) + ((byte & 0x08) << 1)) * 15;
          exp[1] = (((byte & 0x10) >> 4) + ((byte & 0x20) >> 1)) * 15;
          exp[0] = (((byte & 0x40) >> 6) + ((byte & 0x80) >> 3)) * 15;
          break;
      }
      for (j=0; j<scale_y; j++)
        for (i=0; i<scale_x; i++)
          grafyx[(y*scale_y + j)*imageSize.bytes_per_line + x*scale_x + i] = exp[i];
    }
  }
}

void grafyx_write_x(int value)
{
  grafyx_x = value;
}

void grafyx_write_y(int value)
{
  grafyx_y = value;
}

void grafyx_write_data(int value)
{
  grafyx_write_byte(grafyx_x % G_XSIZE, grafyx_y, value);
  if (!(grafyx_mode & G_XNOCLKW)) {
    if (grafyx_mode & G_XDEC)
      grafyx_x--;
    else
      grafyx_x++;
  }
  if (!(grafyx_mode & G_YNOCLKW)) {
    if (grafyx_mode & G_YDEC)
      grafyx_y--;
    else
      grafyx_y++;
  }
}

int grafyx_read_data()
{
  int value = grafyx_unscaled[grafyx_y][grafyx_x % G_XSIZE];

  if (!(grafyx_mode & G_XNOCLKR)) {
    if (grafyx_mode & G_XDEC)
      grafyx_x--;
    else
      grafyx_x++;
  }
  if (!(grafyx_mode & G_YNOCLKR)) {
    if (grafyx_mode & G_YDEC)
      grafyx_y--;
    else
      grafyx_y++;
  }
  return value;
}

void grafyx_write_mode(int value)
{
  const unsigned char old_enable = grafyx_enable;
  const unsigned char old_overlay = grafyx_overlay;

  grafyx_enable = value & G_ENABLE;
  if (grafyx_microlabs)
    grafyx_overlay = (value & G_UL_NOTEXT) == 0;
  grafyx_mode = value;
  trs_screen_640x240((grafyx_enable && !grafyx_overlay) || text80x24);
  if (old_enable != grafyx_enable ||
      (grafyx_enable && old_overlay != grafyx_overlay))
    trs_screen_refresh();
}

void grafyx_write_xoffset(int value)
{
  const unsigned char old_xoffset = grafyx_xoffset;

  grafyx_xoffset = value % G_XSIZE;
  if (grafyx_enable && old_xoffset != grafyx_xoffset)
    trs_screen_refresh();
}

void grafyx_write_yoffset(int value)
{
  const unsigned char old_yoffset = grafyx_yoffset;

  grafyx_yoffset = value;
  if (grafyx_enable && old_yoffset != grafyx_yoffset)
    trs_screen_refresh();
}

void grafyx_write_overlay(int value)
{
  const unsigned char old_overlay = grafyx_overlay;

  grafyx_overlay = value & 1;
  if (grafyx_enable && old_overlay != grafyx_overlay) {
    trs_screen_640x240((grafyx_enable && !grafyx_overlay) || text80x24);
    trs_screen_refresh();
  }
}

int grafyx_get_microlabs()
{
  return grafyx_microlabs;
}

void grafyx_set_microlabs(int on_off)
{
  grafyx_microlabs = on_off;
}

/* Model III MicroLabs support */
void grafyx_m3_reset()
{
  if (grafyx_microlabs) grafyx_m3_write_mode(0);
}

void grafyx_m3_write_mode(int value)
{
  int const enable = (value & G3_ENABLE) != 0;
  int const changed = (enable != grafyx_enable);

  grafyx_enable = enable;
  grafyx_overlay = enable;
  grafyx_mode = value;
  grafyx_y = G3_YLOW(value);
  if (changed) trs_screen_refresh();
}

int grafyx_m3_write_byte(int position, int byte)
{
  if (grafyx_microlabs && (grafyx_mode & G3_COORD)) {
    int const x = (position % 64);
    int const y = (position / 64) * 12 + grafyx_y;

    grafyx_write_byte(x, y, byte);
    return 1;
  } else
    return 0;
}

unsigned char grafyx_m3_read_byte(int position)
{
  if (grafyx_microlabs && (grafyx_mode & G3_COORD)) {
    int const x = (position % 64);
    int const y = (position / 64) * 12 + grafyx_y;

    return grafyx_unscaled[y][x];
  } else
    return trs_screen[position];
}

int grafyx_m3_active()
{
  return (trs_model == 3 && grafyx_microlabs && (grafyx_mode & G3_COORD));
}

/*
 *     The Lowe Electronics LE18 is yet another fairly simple
 *     I/O based 384x192 graphics adapter writing 6bits per
 *     TRS80 character
 *
 *     Port EC (R)
 *     7: goes high for blanking - can spin until high to avoid noise
 *     6: on/off status
 *     5-0: pixel data bit 0 is left
 *
 *     Port ED (W)
 *     7-6: unused
 *     5-0: X position (chars)
 *     Port EE (W)
 *     7-0: Y position (lines)
 *
 *     Port EF (W)
 *     7-1: unused
 *     0: hi res (1 = on)
 */

static unsigned char le18_x, le18_y, le18_on;
int lowe_le18;

void lowe_le18_reset(void)
{
}

void lowe_le18_write_x(int value)
{
  /* This really is 0-255. The unit has 16K x 6bit of RAM
     of which only 12K is displayed. You can use the rest
     as a 4K x 6bit area for .. not a lot really */
  le18_x = value & 63;
}

void lowe_le18_write_y(int value)
{
  le18_y = value;
}

static unsigned char pack8to6(unsigned char c)
{
  return ((c & 0x70) >> 1) | (c & 7);
}

static unsigned char expand6to8(unsigned char c)
{
  unsigned char r;

  r = (c & 0x07);
  if (r & 0x04)
    r |= 0x08;
  r |= (c << 1) & 0x70;
  if (r & 0x40)
    r |= 0x80;
  return r;
}

int lowe_le18_read(void)
{
  if (!lowe_le18)
    return 0xFF;
  return pack8to6(grafyx_unscaled[le18_y][le18_x]) | 0x80
          | ((le18_on) ? 0x40 : 0x00);
}

void lowe_le18_write_data(int value)
{
  if (lowe_le18)
    grafyx_write_byte(le18_x, le18_y, expand6to8(value & 0x3F));
}

void lowe_le18_write_control(int value)
{
  if (lowe_le18 && ((le18_on ^ value) & 1)) {
    le18_on = value & 1;
    grafyx_enable = le18_on;
    grafyx_overlay = le18_on;
    trs_screen_refresh();
  }
}

/*
 * Support for Model I HRG1B 384*192 graphics card
 * (sold in Germany for Model I and Video Genie by RB-Elektronik).
 *
 * Assignment of ports is as follows:
 *    Port 0x00 (out): switch graphics screen off (value ignored).
 *    Port 0x01 (out): switch graphics screen on (value ignored).
 *    Port 0x02 (out): select screen memory address (LSB).
 *    Port 0x03 (out): select screen memory address (MSB).
 *    Port 0x04 (in):  read byte from screen memory.
 *    Port 0x05 (out): write byte to screen memory.
 * (The real hardware decodes only address lines A0-A2 and A7, so
 * that there are several "shadow" ports in the region 0x08-0x7d.
 * However, these undocumented ports are not implemented here.)
 *
 * The 16-bit memory address (port 2 and 3) is used for subsequent
 * read or write operations. It corresponds to a position on the
 * graphics screen, in the following way:
 *    Bits 0-5:   character column address (0-63)
 *    Bits 6-9:   character row address (0-15)
 *                (i.e. bits 0-9 are the "PRINT @" position.)
 *    Bits 10-13: address of line within character cell (0-11)
 *    Bits 14-15: not used
 *
 *      <----port 2 (LSB)---->  <-------port 3 (MSB)------->
 * Bit: 0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15
 *      <-column addr.->  <row addr>  <-line addr.->  <n.u.>
 *
 * Reading from port 4 or writing to port 5 will access six
 * neighbouring pixels corresponding (from left to right) to bits
 * 0-5 of the data byte. Bits 6 and 7 are present in memory, but
 * are ignored.
 *
 * In expanded mode (32 chars per line), the graphics screen has
 * only 192*192 pixels. Pixels with an odd column address (i.e.
 * every second group of 6 pixels) are suppressed.
 */

/* Initialize HRG. */
static void
hrg_init()
{
  unsigned int i;

  /* Precompute arrays of pixel sizes and offsets. */
  for (i = 0; i <= 6; i++) {
    hrg_pixel_x[0][i] = cur_char_width * i / 6;
    hrg_pixel_x[1][i] = cur_char_width*2 * i / 6;
    if (i != 0) {
      hrg_pixel_width[0][i-1] = hrg_pixel_x[0][i] - hrg_pixel_x[0][i-1];
      hrg_pixel_width[1][i-1] = hrg_pixel_x[1][i] - hrg_pixel_x[1][i-1];
    }
  }
  for (i = 0; i <= 12; i++) {
    hrg_pixel_y[i] = cur_char_height * i / 12;
    if (i != 0)
      hrg_pixel_height[i-1] = hrg_pixel_y[i] - hrg_pixel_y[i-1];
  }
  if (cur_char_width % 6 != 0 || cur_char_height % 12 != 0)
    error("character size %d*%d not a multiple of 6*12 HRG raster",
        cur_char_width, cur_char_height);
}

/* Switch HRG on (1) or off (0). */
void
hrg_onoff(int enable)
{
  static int init = 0;

  if ((hrg_enable!=0) == (enable!=0)) return; /* State does not change. */

  if (!init) {
    hrg_init();
    init = 1;
  }
  hrg_enable = enable;
  trs_screen_refresh();
}

/* Write address to latch. */
void
hrg_write_addr(int addr, int mask)
{
  hrg_addr = (hrg_addr & ~mask) | (addr & mask);
}

/* Write byte to HRG memory. */
void
hrg_write_data(int data)
{
  int old_data;
  int position, line;
  int bits0, bits1;

  if (hrg_addr >= HRG_MEMSIZE) return; /* nonexistent address */
  old_data = hrg_screen[hrg_addr];
  hrg_screen[hrg_addr] = data;

  if (!hrg_enable) return;
  if ((currentmode & EXPANDED) && (hrg_addr & 1)) return;
  if ((data &= 0x3f) == (old_data &= 0x3f)) return;

  position = hrg_addr & 0x3ff; /* bits 0-9: "PRINT @" screen position */
  line = hrg_addr >> 10;       /* vertical offset inside character cell */
  bits0 = ~data & old_data;    /* pattern to clear */
  bits1 = data & ~old_data;    /* pattern to set */

  if (bits0 == 0
      || trs_screen[position] == 0x20
      || trs_screen[position] == 0x80
      /*|| (trs_screen[position] < 0x80 && line >= 8 && !usefont)*/
     ) {
    /* Only additional bits set, or blank text character.
       No need for update of text. */
    int const destx = (position % row_chars) * cur_char_width + left_margin;
    int const desty = (position / row_chars) * cur_char_height + top_margin
      + hrg_pixel_y[line];
    int const *x = hrg_pixel_x[(currentmode&EXPANDED)!=0];
    int const *w = hrg_pixel_width[(currentmode&EXPANDED)!=0];
    int const h = hrg_pixel_height[line];
    int n0 = 0;
    int n1 = 0;
    int flag = 0;
    int i, j, b;
    SDL_Rect rect0[3];    /* 6 bits => max. 3 groups of adjacent "0" bits */
    SDL_Rect rect1[3];

    /* Compute arrays of rectangles to clear and to set. */
    for (j = 0, b = 1; j < 6; j++, b <<= 1) {
      if (bits0 & b) {
        if (flag >= 0) {       /* Start new rectangle. */
          rect0[n0].x = destx + x[j];
          rect0[n0].y = desty;
          rect0[n0].w = w[j];
          rect0[n0].h = h;
          n0++;
          flag = -1;
        }
        else {                 /* Increase width of rectangle. */
          rect0[n0-1].w += w[j];
        }
      }
      else if (bits1 & b) {
        if (flag <= 0) {
          rect1[n1].x = destx + x[j];
          rect1[n1].y = desty;
          rect1[n1].w = w[j];
          rect1[n1].h = h;
          n1++;
          flag = 1;
        }
        else {
          rect1[n1-1].w += w[j];
        }
      }
      else {
        flag = 0;
      }
    }
    for (i=0;i<n0;i++)
      SDL_FillRect(screen, &rect0[i], background);
    for (i=0;i<n1;i++)
      SDL_FillRect(screen, &rect0[i], foreground);
  }
  else {
    /* Unfortunately, HRG1B combines text and graphics with an
       (inclusive) OR. Thus, in the general case, we cannot erase
       the old graphics byte without losing the text information.
       Call trs_screen_write_char to restore the text character
       (erasing the graphics). This function will in turn call
       hrg_update_char and restore 6*12 graphics pixels. Sigh. */
    trs_screen_write_char(position, trs_screen[position]);
  }
}

/* Read byte from HRG memory. */
int
hrg_read_data()
{
  if (hrg_addr >= HRG_MEMSIZE) return 0xff; /* nonexistent address */
  return hrg_screen[hrg_addr];
}

/* Update graphics at given screen position.
   Called by trs_screen_write_char. */
static void
hrg_update_char(int position)
{
  int const destx = (position % row_chars) * cur_char_width + left_margin;
  int const desty = (position / row_chars) * cur_char_height + top_margin;
  int const *x = hrg_pixel_x[(currentmode&EXPANDED)!=0];
  int const *w = hrg_pixel_width[(currentmode&EXPANDED)!=0];
  int byte;
  int prev_byte = 0;
  int n = 0;
  int np = 0;
  int i, j, flag;
  SDL_Rect rect[3*12];

  /* Compute array of rectangles. */
  for (i = 0; i < 12; i++) {
    if ((byte = hrg_screen[position+(i<<10)] & 0x3f) == 0) {
    }
    else if (byte != prev_byte) {
      np = n;
      flag = 0;
      for (j = 0; j < 6; j++) {
        if (!(byte & 1<<j)) {
          flag = 0;
        }
        else if (!flag) {     /* New rectangle. */
          rect[n].x = destx + x[j];
          rect[n].y = desty + hrg_pixel_y[i];
          rect[n].w = w[j];
          rect[n].h = hrg_pixel_height[i];
          n++;
          flag = 1;
        }
        else {                /* Increase width. */
          rect[n-1].w += w[j];
        }
      }
    }
    else {                    /* Increase heights. */
      for (j = np; j < n; j++)
        rect[j].h += hrg_pixel_height[i];
    }
    prev_byte = byte;
  }
  for (i=0;i<n;i++)
    SDL_FillRect(screen, &rect[i], foreground);
}


void trs_get_mouse_pos(int *x, int *y, unsigned int *buttons)
{
  int win_x, win_y;
  Uint8 mask;

  mask = SDL_GetMouseState(&win_x, &win_y);
#if MOUSEDEBUG
  debug("get_mouse %d %d 0x%x ->", win_x, win_y, mask);
#endif
  if (win_x >= 0 && win_x < OrigWidth &&
      win_y >= 0 && win_y < OrigHeight) {
    /* Mouse is within emulator window */
    if (win_x < left_margin) win_x = left_margin;
    if (win_x >= OrigWidth - left_margin) win_x = OrigWidth - left_margin - 1;
    if (win_y < top_margin) win_y = top_margin;
    if (win_y >= OrigHeight - top_margin) win_y = OrigHeight - top_margin - 1;
    *x = mouse_last_x = (win_x - left_margin)
      * mouse_x_size
      / (OrigWidth - 2*left_margin);
    *y = mouse_last_y = (win_y - top_margin)
      * mouse_y_size
      / (OrigHeight - 2*top_margin);
    mouse_last_buttons = 7;
    /* !!Note: assuming 3-button mouse */
    if (mask & SDL_BUTTON(SDL_BUTTON_LEFT))   mouse_last_buttons &= ~4;
    if (mask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) mouse_last_buttons &= ~2;
    if (mask & SDL_BUTTON(SDL_BUTTON_RIGHT))  mouse_last_buttons &= ~1;
  }
  *x = mouse_last_x;
  *y = mouse_last_y;
  *buttons = mouse_last_buttons;
#if MOUSEDEBUG
  debug("%d %d 0x%x\n",
      mouse_last_x, mouse_last_y, mouse_last_buttons);
#endif
}

void trs_set_mouse_pos(int x, int y)
{
  int dest_x, dest_y;

  if (x == mouse_last_x && y == mouse_last_y) {
    /* Kludge: Ignore warp if it says to move the mouse to where we
       last said it was. In general someone could really want to do that,
       but with MDRAW, gratuitous warps to the last location occur frequently.
    */
    return;
  }
  dest_x = left_margin + x * (OrigWidth - 2*left_margin) / mouse_x_size;
  dest_y = top_margin  + y * (OrigHeight - 2*top_margin) / mouse_y_size;

#if MOUSEDEBUG
  debug("set_mouse %d %d -> %d %d\n", x, y, dest_x, dest_y);
#endif
#ifdef SDL2
  SDL_WarpMouseInWindow(window, dest_x, dest_y);
#else
  SDL_WarpMouse(dest_x, dest_y);
#endif
}

void trs_get_mouse_max(int *x, int *y, unsigned int *sens)
{
  *x = mouse_x_size - (mouse_old_style ? 0 : 1);
  *y = mouse_y_size - (mouse_old_style ? 0 : 1);
  *sens = mouse_sens;
}

void trs_set_mouse_max(int x, int y, unsigned int sens)
{
  if ((x & 1) == 0 && (y & 1) == 0) {
    /* "Old style" mouse drivers took the size here; new style take
       the maximum. As a heuristic kludge, we assume old style if
       the values are even, new style if not. */
    mouse_old_style = 1;
  }
  mouse_x_size = x + (mouse_old_style ? 0 : 1);
  mouse_y_size = y + (mouse_old_style ? 0 : 1);
  mouse_sens = sens;
}

int trs_get_mouse_type()
{
  /* !!Note: assuming 3-button mouse */
  return 1;
}

void trs_main_save(FILE *file)
{
  unsigned int i;

  trs_save_int(file,&trs_model,1);
  trs_save_uchar(file,trs_screen,2048);
  trs_save_int(file,&screen_chars,1);
  trs_save_int(file,&col_chars,1);
  trs_save_int(file,&row_chars,1);
  trs_save_int(file,&currentmode,1);
  trs_save_int(file,&text80x24,1);
  trs_save_int(file,&screen640x240,1);
  trs_save_int(file,&trs_charset,1);
  trs_save_int(file,&trs_charset1,1);
  trs_save_int(file,&trs_charset3,1);
  trs_save_int(file,&trs_charset4,1);
  for (i=0;i<G_YSIZE;i++)
    trs_save_uchar(file,grafyx_unscaled[i],G_XSIZE);
  trs_save_uchar(file,&grafyx_x,1);
  trs_save_uchar(file,&grafyx_y,1);
  trs_save_uchar(file,&grafyx_enable,1);
  trs_save_uchar(file,&grafyx_overlay,1);
  trs_save_uchar(file,&grafyx_xoffset,1);
  trs_save_uchar(file,&grafyx_yoffset,1);
  trs_save_uchar(file,&grafyx_x,1);
  trs_save_int(file,key_queue,KEY_QUEUE_SIZE);
  trs_save_int(file,&key_queue_head,1);
  trs_save_int(file,&key_queue_entries,1);
  trs_save_int(file,&lowe_le18,1);
  trs_save_int(file,&lowercase,1);
  trs_save_int(file,&stringy,1);
}

void trs_main_load(FILE *file)
{
  unsigned int i;

  trs_load_int(file,&trs_model,1);
  trs_load_uchar(file,trs_screen,2048);
  trs_load_int(file,&screen_chars,1);
  trs_load_int(file,&col_chars,1);
  trs_load_int(file,&row_chars,1);
  trs_load_int(file,&currentmode,1);
  trs_load_int(file,&text80x24,1);
  trs_load_int(file,&screen640x240,1);
  trs_load_int(file,&trs_charset,1);
  trs_load_int(file,&trs_charset1,1);
  trs_load_int(file,&trs_charset3,1);
  trs_load_int(file,&trs_charset4,1);
  for (i=0;i<G_YSIZE;i++)
    trs_load_uchar(file,grafyx_unscaled[i],G_XSIZE);
  trs_load_uchar(file,&grafyx_x,1);
  trs_load_uchar(file,&grafyx_y,1);
  trs_load_uchar(file,&grafyx_enable,1);
  trs_load_uchar(file,&grafyx_overlay,1);
  trs_load_uchar(file,&grafyx_xoffset,1);
  trs_load_uchar(file,&grafyx_yoffset,1);
  trs_load_uchar(file,&grafyx_x,1);
  trs_load_int(file,key_queue,KEY_QUEUE_SIZE);
  trs_load_int(file,&key_queue_head,1);
  trs_load_int(file,&key_queue_entries,1);
  trs_load_int(file,&lowe_le18,1);
  trs_load_int(file,&lowercase,1);
  trs_load_int(file,&stringy,1);
}

int trs_sdl_savebmp(const char *filename)
{
  return SDL_SaveBMP(screen, filename);
}
