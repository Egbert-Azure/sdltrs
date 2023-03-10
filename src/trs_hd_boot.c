/* Patch Level II ROM for auto-boot from hard disk:
 * https://trs80.nl/software/model1hddboot/
 */

static void trs_boot_hd(void)
{
  rom_write(0x0066, 0xAF);  /* XOR A */
  rom_write(0x0067, 0x3D);  /* DEC A */
  rom_write(0x0068, 0x18);  /* JR 0000 */
  rom_write(0x0069, 0x98);
  rom_write(0x006A, 0xD3);  /* OUT(0CF),A */
  rom_write(0x006B, 0xCF);
  rom_write(0x006C, 0xDB);  /* IN A,(0CF) */
  rom_write(0x006D, 0xCF);
  rom_write(0x006E, 0xE6);  /* AND C0 */
  rom_write(0x006F, 0xC0);
  rom_write(0x0070, 0xEE);  /* XOR 40 */
  rom_write(0x0071, 0x40);
  rom_write(0x0072, 0x20);  /* JRNZ, 006C */
  rom_write(0x0073, 0xF8);
  rom_write(0x0074, 0xC9);  /* RET */
  rom_write(0x0075, 0x11);  /* LD DE,4080 */
  rom_write(0x0076, 0x80);
  rom_write(0x0077, 0x40);

  rom_write(0x00B5, 0x21);  /* LD HL,2FFB */
  rom_write(0x00B6, 0xFB);
  rom_write(0x00B7, 0x2F);

  rom_write(0x00FC, 0x21);  /* LD HL,0473 */
  rom_write(0x00FD, 0x73);
  rom_write(0x00FE, 0x04);

  rom_write(0x0105, 0xCD);  /* CALL 006C */
  rom_write(0x0106, 0x6C);
  rom_write(0x0107, 0x00);
  rom_write(0x0108, 0x01);  /* LD BC, 06CE */
  rom_write(0x0109, 0xCE);
  rom_write(0x010A, 0x06);
  rom_write(0x010B, 0xED);  /* OUT (C),A */
  rom_write(0x010C, 0x79);
  rom_write(0x010D, 0x0D);  /* DEC C */
  rom_write(0x010E, 0x10);  /* DJNZ 010B */
  rom_write(0x010F, 0xFB);
  rom_write(0x0110, 0x3E);  /* LD A, 20 */
  rom_write(0x0111, 0x20);
  rom_write(0x0112, 0xCD);  /* CALL 006A */
  rom_write(0x0113, 0x6A);
  rom_write(0x0114, 0x00);
  rom_write(0x0115, 0x21);  /* LD HL,4200 */
  rom_write(0x0116, 0x00);
  rom_write(0x0117, 0x42);
  rom_write(0x0118, 0xE5);  /* PUSH HL */
  rom_write(0x0119, 0xED);
  rom_write(0x011A, 0xB2);  /* INIR */
  rom_write(0x011B, 0xC9);  /* RET */

  rom_write(0x0471, 0x18);  /* JR 047D */
  rom_write(0x0472, 0x0A);
  rom_write(0x0473, 0x52);  /* DEFM 'R/S' */
  rom_write(0x0474, 0x2F);
  rom_write(0x0475, 0x53);
  rom_write(0x0476, 0x20);  /* DEFM ' L2' */
  rom_write(0x0477, 0x4C);
  rom_write(0x0478, 0x32);
  rom_write(0x0479, 0x2E);  /* DEFM '..' */
  rom_write(0x047A, 0x2E);
  rom_write(0x047B, 0x0D);  /* DEFB 0D */
  rom_write(0x047C, 0x00);  /* DEFB 00 */
  rom_write(0x047D, 0xCD);  /* CALL 0541 */
  rom_write(0x047E, 0x41);
  rom_write(0x047F, 0x05);

  rom_write(0x2FFB, 0x4D);  /* DEFM 'Mem' */
  rom_write(0x2FFC, 0x65);
  rom_write(0x2FFD, 0x6D);
  rom_write(0x2FFE, 0x2E);  /* DEFM '.' */
  rom_write(0x2FFF, 0x00);  /* DEFB 00 */

  rom_write(0x0674, 0x08);  /* EX AF, AF' */
  rom_write(0x0675, 0xAF);  /* XOR A */
  rom_write(0x0676, 0xD3);  /* OUT (0FF),A */
  rom_write(0x0677, 0xFF);
  rom_write(0x0678, 0x21);  /* LD HL,06D2 */
  rom_write(0x0679, 0xD2);
  rom_write(0x067A, 0x06);
  rom_write(0x067B, 0x11);  /* LD DE,4000 */
  rom_write(0x067C, 0x00);
  rom_write(0x067D, 0x40);
  rom_write(0x067E, 0x01);  /* LD BC, 0036 */
  rom_write(0x067F, 0x36);
  rom_write(0x0680, 0x00);
  rom_write(0x0681, 0xED);  /* LDIR */
  rom_write(0x0682, 0xB0);
  rom_write(0x0683, 0x3D);  /* DEC A */
  rom_write(0x0684, 0x20);  /* JR NZ,0678 */
  rom_write(0x0685, 0xF2);
  rom_write(0x0686, 0x06);  /* LD B,27 */
  rom_write(0x0687, 0x27);
  rom_write(0x0688, 0x12);  /* LD (DE), A */
  rom_write(0x0689, 0x13);  /* INC DE */
  rom_write(0x068A, 0x10);  /* DJNZ 0688 */
  rom_write(0x068B, 0xFC);
  rom_write(0x068C, 0x31);  /* LD SP, 407D */
  rom_write(0x068D, 0x7D);
  rom_write(0x068E, 0x40);
  rom_write(0x068F, 0x3A);  /* LD A,(3840) */
  rom_write(0x0690, 0x40);
  rom_write(0x0691, 0x38);
  rom_write(0x0692, 0xCB);  /* BIT 2, A */
  rom_write(0x0693, 0x57);
  rom_write(0x0694, 0x20);  /* JRNZ, 06C8 */
  rom_write(0x0695, 0x32);
  rom_write(0x0696, 0x17);  /* RLA */
  rom_write(0x0697, 0x38);  /* JR C,069F */
  rom_write(0x0698, 0x06);
  rom_write(0x0699, 0xDB);  /* IN A,(0CF) */
  rom_write(0x069A, 0xCF);
  rom_write(0x069B, 0x3C);  /* INC A */
  rom_write(0x069C, 0xC2);  /* JP NZ,0105 */
  rom_write(0x069D, 0x05);
  rom_write(0x069E, 0x01);
  rom_write(0x069F, 0x21);  /* LD HL,37EC */
  rom_write(0x06A0, 0xEC);
  rom_write(0x06A1, 0x37);
  rom_write(0x06A2, 0x7E);  /* LD A,(HL) */
  rom_write(0x06A3, 0x3C);  /* INC A */
  rom_write(0x06A4, 0x28);  /* JR Z, 06C8 */
  rom_write(0x06A5, 0x22);
  rom_write(0x06A6, 0x3E);  /* LD A, 01 */
  rom_write(0x06A7, 0x01);
  rom_write(0x06A8, 0x32);  /* LD(37E1), A */
  rom_write(0x06A9, 0xE1);
  rom_write(0x06AA, 0x37);
  rom_write(0x06AB, 0x11);  /* LD DE,37EE */
  rom_write(0x06AC, 0xEE);
  rom_write(0x06AD, 0x37);
  rom_write(0x06AE, 0x36);  /* LD (HL),3 */
  rom_write(0x06AF, 0x03);
  rom_write(0x06B0, 0xCD);  /* CALL 0060 */
  rom_write(0x06B1, 0x60);
  rom_write(0x06B2, 0x00);
  rom_write(0x06B3, 0xCB);  /* BIT 0,(HL) */
  rom_write(0x06B4, 0x46);
  rom_write(0x06B5, 0x20);  /* JR NZ, 06B3 */
  rom_write(0x06B6, 0xFC);
  rom_write(0x06B7, 0x12);  /* LD (DE),A */
  rom_write(0x06B8, 0x13);  /* INC DE */
  rom_write(0x06B9, 0x06);  /* LD B,42 */
  rom_write(0x06BA, 0x42);
  rom_write(0x06BB, 0xC5);  /* PUSH BC */
  rom_write(0x06BC, 0x36);  /* LD (HL),8C> */
  rom_write(0x06BD, 0x8C);
  rom_write(0x06BE, 0xCB);  /* BIT 1, (HL) */
  rom_write(0x06BF, 0x4E);
  rom_write(0x06C0, 0x28);  /* JR Z,06BE */
  rom_write(0x06C1, 0xFC);
  rom_write(0x06C2, 0x1A);  /* LD A,(DE) */
  rom_write(0x06C3, 0x02);  /* LD (BC), A */
  rom_write(0x06C4, 0x0C);  /* INC C */
  rom_write(0x06C5, 0x20);  /* JR NZ, 06BE */
  rom_write(0x06C6, 0xF7);
  rom_write(0x06C7, 0xC9);  /* RET */
  rom_write(0x06C8, 0x08);  /* EX AF, AF' */
  rom_write(0x06C9, 0xCA);  /* JP Z,0075 */
  rom_write(0x06CA, 0x75);
  rom_write(0x06CB, 0x00);
}
