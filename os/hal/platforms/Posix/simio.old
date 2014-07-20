#include "ch.h"
#include "hal.h"
#include "simio.h"

#include <stdio.h>

#define SHELL_WA_SIZE       THD_WA_SIZE(256)
#define CONSOLE_WA_SIZE     THD_WA_SIZE(256)

static EventListener sd1fel;
static EventListener sd2fel;
static Thread *shelltp1;
static Thread *shelltp2;

struct {
  simio_cb_t  fp;
  void*       arg;
} input_cb;

BaseSequentialStream *output_stream = NULL;

/**
 * @brief   Shell termination event source.
 */
EventSource shell_terminated;

/**
 * @brief   Shell thread function.
 *
 * @param[in] p         pointer to a @p BaseSequentialStream object
 * @return              Termination reason.
 * @retval RDY_OK       terminated by command.
 * @retval RDY_RESET    terminated by reset condition on the I/O channel.
 *
 * @notapi
 */
static msg_t input_handler(void *vptr) {
  BaseSequentialStream *chp = vptr;
  char buf[SIM_INPUT_MAX];
  uint32_t i;

  for (i = 0; i < sizeof buf - 1; i++) {
    uint8_t c;
    if (chSequentialStreamRead(chp, &c, 1) == 0)
      return 0;
    if (c == '\n')
      break;
    buf[i] = c;
  }
  buf[i] = '\0';

  input_cb.fp(buf, input_cb.arg);

  /* Atomically broadcasting the event source and terminating the thread,
     there is not a chSysUnlock() because the thread terminates upon return.*/
  chSysLock();
  chEvtBroadcastI(&shell_terminated);
  chThdExitS(RDY_OK);

  /* Never executed, silencing a warning.*/
  return 0;
}

void sim_output_puts(unsigned char *buf, uint32_t size) {
  unsigned char *bufp = buf;
  uint32_t nbyts, left = size;

  if (!output_stream)
    return;

  while (left) {
    if ((nbyts = chSequentialStreamWrite(output_stream, bufp, left)) <= 0)
      break;
    bufp += nbyts;
    left -= nbyts;
  }
}

/**
 * @brief SD1 status change handler.
 *
 * @param[in] id event id.
 */
static void sd1_handler(eventid_t id) {
  flagsmask_t flags;

  (void)id;
  flags = chEvtGetAndClearFlags(&sd1fel);
  if ((flags & CHN_CONNECTED) && (shelltp1 == NULL)) {
    printf("Init: connection on SD1\n");
    shelltp1 = chThdCreateFromHeap(NULL, SHELL_WA_SIZE, NORMALPRIO + 10, input_handler, (void *)&SD1);
  }
  if (flags & CHN_DISCONNECTED) {
    printf("Init: disconnection on SD1\n");
    chSysLock();
    chIQResetI(&SD1.iqueue);
    chSysUnlock();
  }
}

/**
 * @brief SD1 status change handler.
 *
 * @param[in] id event id.
 */
static void sd2_handler(eventid_t id) {
  flagsmask_t flags;

  (void)id;
  flags = chEvtGetAndClearFlags(&sd2fel);
  if ((flags & CHN_CONNECTED) && (shelltp2 == NULL)) {
    printf("Init: connection on SD2\n");
    output_stream = (BaseSequentialStream*)&SD2;
  }
  if (flags & CHN_DISCONNECTED) {
    printf("Init: disconnection on SD2\n");
    output_stream = NULL;
    chSysLock();
    chIQResetI(&SD2.iqueue);
    chSysUnlock();
  }
}

/**
 * @brief Shell termination handler.
 *
 * @param[in] id event id.
 */
static void termination_handler(eventid_t id) {
  (void)id;
  if (shelltp1 && chThdTerminated(shelltp1)) {
    chThdWait(shelltp1);
    shelltp1 = NULL;
    chThdSleepMilliseconds(10);
    // printf("Init: shell on SD1 terminated\n");
    chSysLock();
    chOQResetI(&SD1.oqueue);
    chSysUnlock();
  }

  if (shelltp2 && chThdTerminated(shelltp2)) {
    chThdWait(shelltp2);
    shelltp2 = NULL;
    chThdSleepMilliseconds(10);
    // cputs("Init: shell on SD2 terminated");
    chSysLock();
    chOQResetI(&SD2.oqueue);
    chSysUnlock();
  }
}

static evhandler_t fhandlers[] = {
  termination_handler,
  sd1_handler,
  sd2_handler
};

static msg_t event_thread(void *arg) {
  (void)arg;
  EventListener tel;

  chEvtInit(&shell_terminated);
  chEvtRegister(&shell_terminated, &tel, 0);

  chEvtRegister(chnGetEventSource(&SD1), &sd1fel, 1);
  chEvtRegister(chnGetEventSource(&SD2), &sd2fel, 2);

  /*
   * Events servicing loop.
   */
  while (!chThdShouldTerminate())
    chEvtDispatch(fhandlers, chEvtWaitOne(ALL_EVENTS));

  /*
   * Clean simulator exit.
   */
  chEvtUnregister(chnGetEventSource(&SD1), &sd1fel);

  return 0;
}

void spawn_event_thread(void) {
  static int created = FALSE;
  if (!created)
    (void)chThdCreateFromHeap(NULL, CONSOLE_WA_SIZE, NORMALPRIO,
                              event_thread, NULL);
  created = TRUE;
}

void sim_io_init(uint32_t porti, uint32_t porto) {
  SerialConfig config = {porti, porto};
  if (porti)
    sdStart(&SD1, &config);
  if (porto)
    sdStart(&SD2, &config);
}

void sim_input_cb(simio_cb_t cb, void *arg) {
  input_cb.fp = cb;
  input_cb.arg = arg;

  spawn_event_thread();
}
