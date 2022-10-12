#include <stdio.h>
#include "trs_clones.h"
#include "trs_state_save.h"

struct clone_quirks clone_quirks;
static const struct clone_quirks quirks[] = {
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

  current_clone = clone;

  switch (clone) {
    case CP500:
      clone_quirks = quirks[1];
      break;
    case CP500_M80:
      clone_quirks = quirks[2];
      break;
    case EG3200:
      clone_quirks = quirks[3];
      break;
    case GENIE3S:
      clone_quirks = quirks[4];
      break;
    case LNW80:
      clone_quirks = quirks[5];
      break;
    case SPEEDMASTER:
      clone_quirks = quirks[6];
      break;
    default:
      clone_quirks = quirks[0];
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
