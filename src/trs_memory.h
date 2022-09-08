#ifndef _TRS_MEMORY_H
#define _TRS_MEMORY_H

/* Locations for Model I, Model III, and Model 4 map 0 */
#define VIDEO_START     (0x3c00)
#define PRINTER_ADDRESS (0x37E8)
#define KEYBOARD_START  (0x3800)
#define RAM_START       (0x4000)

int  trs80_model3_mem_read(int address);
void trs80_model3_mem_write(int address, int value);

void mem_video_page(int offset);
Uint8 mem_video_page_read(int vaddr);
int mem_video_page_write(int vaddr, Uint8 value);

#endif
