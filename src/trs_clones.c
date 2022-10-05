#include <stdio.h>
#include "trs_clones.h"
#include "trs_state_save.h"

#define CLONES 7

struct model_quirks model_quirks;
static const struct model_quirks quirks[CLONES] = {
  /*
   *                                       +--------- use bit 4 for disk side select
   * Clone ID    name of clone             |  +------ do not fire early disk interrupts
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

static int selected_clone;

void trs_clone_quirks(int clone)
{
  if (clone >= 0 && clone < CLONES) {
    model_quirks = quirks[clone];
    selected_clone = clone;
  }
}

void trs_clone_save(FILE *file)
{
  trs_save_int(file, &selected_clone, 1);
}

void trs_clone_load(FILE *file)
{
  trs_load_int(file, &selected_clone, 1);
  trs_clone_quirks(selected_clone);
}
