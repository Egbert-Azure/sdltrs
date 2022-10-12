#ifndef _TRS_CLONES_H
#define _TRS_CLONES_H

/* Clones */
#define CP500       (1 << 1)
#define CP500_M80   (1 << 2)
#define EG3200      (1 << 3)
#define GENIE3S     (1 << 4)
#define LNW80       (1 << 5)
#define SPEEDMASTER (1 << 6)

/*
 * Some clones contain seem to behave in a slightly different way
 * than stock TRS-80 models:
 */
struct clone_quirks {
  /*
   * Clone:
   */
  int clone;
  /*
   * Model name of the clone:
   */
  char *name;

  /*
   * In Model III, when port F4 is written to, and bit 0x40 is enabled,
   * and there a disk event pending, then the emulator triggers an
   * interrupt immediately instead of when the event deadline arrives.
   *
   * However, there are known instances of code that break due to this
   * behaviour, such as SO-08 running on CP-500, because the interrupt
   * is then triggered too early and aborts the execution earlier than
   * the code is expecting.
   *
   * Therefore, allow the behaviour to be disabled for some clones:
   */
  int disk_select_ignore_wait;

  /**
   * In Model III, when the timer fires, the timer bit in the interrupt
   * latch turns on and stays on, even when code reads the current state
   * of the latch via the E0 port. This means that once the timer fires,
   * and the interrupt handler is called, disables interrupts, reads E0,
   * processes the interrupt, then re-enable interrupts and returns,
   * the timer interrupt will trigger immediately as it is latched.
   *
   * This behaviour breaks SO-08 running on CP-500, as it will cause the
   * interrupt to keep firing, preventing normal code from progressing
   * since the interrupt handler is re-invoked in an infinite loop.
   *
   * Therefore, allow making reading the port clear the timer latch:
   */
  int interrupt_latch_clears_timer;
};

extern struct clone_quirks clone_quirks;
extern void trs_clone_quirks(int clone);

#endif
