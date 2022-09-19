#ifndef _TRS_MEMORY_H
#define _TRS_MEMORY_H

/* Locations for Model I, Model III, and Model 4 map 0 */
#define VIDEO_START     (0x3c00)
#define PRINTER_ADDRESS (0x37E8)
#define KEYBOARD_START  (0x3800)
#define RAM_START       (0x4000)

/* Memory Expansion Cards */
#define GENIEPLUS       (1) /* EACA EG 3200 Genie III 384 KB */
#define HUFFMAN         (2) /* Dave Huffman (and other) 2 MB (4/4P) */
#define HYPERMEM        (3) /* Anitek HyperMem 1 MB (4/4P) */
#define RAM192B         (4) /* TCS Genie IIs/SpeedMaster 768 KB */
#define SUPERMEM        (5) /* AlphaTech SuperMem 512 KB (I/III) */

int  trs80_model3_mem_read(int address);
void trs80_model3_mem_write(int address, int value);

void mem_video_page(int offset);
Uint8 mem_video_read(int vaddr);
Uint8 mem_video_page_read(int vaddr);
int mem_video_write(int vaddr, Uint8 value);
int mem_video_page_write(int vaddr, Uint8 value);

#endif
