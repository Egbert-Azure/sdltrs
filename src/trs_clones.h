#ifndef _TRS_CLONES_H
#define _TRS_CLONES_H

/*
 * Some clones contain seem to behave in a slightly different way
 * than stock TRS-80 models:
 */
struct model_quirks {
  /*
   * Model name of the clone:
   */
  char *clone;

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

extern struct model_quirks model_quirks;
static const struct model_quirks no_model_quirks;

#endif
