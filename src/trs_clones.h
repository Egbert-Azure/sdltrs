#ifndef _TRS_CLONES_H
#define _TRS_CLONES_H

/* Clones */
#define CP500       (1 << 1)
#define CP500_M80   (1 << 2)
#define EG3200      (1 << 3)
#define GENIE3S     (1 << 4)
#define LNW80       (1 << 5)
#define SPEEDMASTER (1 << 6)

struct clone_quirks {
  /*
   * Clone Model:
   */
  int model;
  /*
   * Name of the clone:
   */
  char *name;
};

extern struct clone_quirks clone_quirks;
extern void trs_clone_quirks(int clone);

#endif
