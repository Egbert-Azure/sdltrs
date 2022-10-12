#include <stdio.h>
#include "trs_clones.h"
#include "trs_state_save.h"

#define CLONES 7

struct clone_quirks clone_quirks;
static const struct clone_quirks quirks[CLONES] = {
  /*
   *                                       +--------- use bit 4 for disk side select
   * Clone       name of clone             |  +------ do not fire early disk interrupts
   *                                       |  |  +--- clear timer when latch is read
   *                                       |  |  |  */
  { 0,           NULL,                     0, 0, 0, },
  { CP500,       "CP-500",                 0, 0, 0, },
  { CP500_M80,   "CP-500 M80",             0, 1, 1, },
  { EG3200,      "EACA EG 3200 Genie III", 1, 0, 0, },
  { GENIE3S,     "TCS Genie IIIs",         1, 0, 0, },
  { LNW80,       "LNW80",                  0, 0, 0, },
  { SPEEDMASTER, "TCS SpeedMaster",        1, 0, 0, },
};

static int current_clone;

void trs_clone_quirks(int clone)
{
  if (clone == current_clone)
    return;

  if (clone >= 0 && clone < CLONES) {
    clone_quirks = quirks[clone];
    current_clone = clone;
  }
}

void trs_clone_save(FILE *file)
{
  trs_save_int(file, &current_clone, 1);
}

void trs_clone_load(FILE *file)
{
  trs_load_int(file, &current_clone, 1);
  trs_clone_quirks(current_clone);
}
