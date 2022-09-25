#include "trs_clones.h"

struct model_quirks model_quirks;
static const struct model_quirks no_model_quirks;

static const struct model_quirks cp500_quirks = {
    "CP-500",
    0,
    0,
};

static const struct model_quirks cp500_m80_quirks = {
    "CP-500 M80", /* name */
    1, /* do not fire early disk interrupts */
    1, /* clear timer when latch is read */
};

static const struct model_quirks eg3200_quirks = {
    "EACA EG 3200 Genie III",
    0,
    0,
};

static const struct model_quirks genie3s_quirks = {
    "TCS Genie IIIs",
    0,
    0,
};

static const struct model_quirks lnw80_quirks = {
    "LNW80",
    0,
    0,
};

static const struct model_quirks speedmaster_quirks = {
    "TCS SpeedMaster",
    0,
    0,
};

void trs_clone_quirks(int clone)
{
  switch (clone) {
    case 0:
      model_quirks = no_model_quirks;
      break;
    case CP500:
      model_quirks = cp500_quirks;
      break;
    case CP500_M80:
      model_quirks = cp500_m80_quirks;
      break;
    case EG3200:
      model_quirks = eg3200_quirks;
      break;
    case GENIE3S:
      model_quirks = genie3s_quirks;
      break;
    case LNW80:
      model_quirks = lnw80_quirks;
      break;
    case SPEEDMASTER:
      model_quirks = speedmaster_quirks;
      break;
  }
}
