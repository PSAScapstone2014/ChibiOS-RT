#include "ch.h"
#include "hal.h"
#include "simio.h"
#include "chprintf.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#define THD_SIZE 256

static SerialConfig sdcfg;
static EventListener sd1fel;
static EventListener sd2fel;

static Thread *itp;
static Thread *otp;
static Thread *etp;

static struct {
  simio_cb_t  fp;
  void*       arg;
} input_cb;

BaseSequentialStream *output_stream = NULL;

/**
 * @brief   IO termination event source.
 */
EventSource io_terminated;

/**
 * @brief   Reads a text line from input and sends it to the registered callback.
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
  chEvtBroadcastI(&io_terminated);
  chThdExitS(RDY_OK);

  /* Never executed, silencing a warning.*/
  return 0;
}

void sim_printf(char *fmt, ...) {
  va_list ap;

  if (!output_stream)
    return;

  va_start(ap, fmt);
  chvprintf(output_stream, fmt, ap);
  va_end(ap);
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
  if ((flags & CHN_CONNECTED) && (itp == NULL)) {
    itp = chThdCreateFromHeap(NULL, THD_WA_SIZE(THD_SIZE), NORMALPRIO + 10, input_handler, (void *)&SD1);
  }
  if (flags & CHN_DISCONNECTED) {
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
  if ((flags & CHN_CONNECTED) && (otp == NULL)) {
    output_stream = (BaseSequentialStream*)&SD2;
  }
  if (flags & CHN_DISCONNECTED) {
    output_stream = NULL;
    chSysLock();
    chIQResetI(&SD2.iqueue);
    chSysUnlock();
  }
}

/**
 * @brief IO termination handler.
 *
 * @param[in] id event id.
 */
static void termination_handler(eventid_t id) {
  (void)id;
  if (itp && chThdTerminated(itp)) {
    chThdWait(itp);
    itp = NULL;
    chThdSleepMilliseconds(10);
    chSysLock();
    chOQResetI(&SD1.oqueue);
    chSysUnlock();
  }

  if (otp && chThdTerminated(otp)) {
    chThdWait(otp);
    otp = NULL;
    chThdSleepMilliseconds(10);
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

  chEvtInit(&io_terminated);
  chEvtRegister(&io_terminated, &tel, 0);

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
  chEvtUnregister(chnGetEventSource(&SD2), &sd2fel);

  return 0;
}

void sim_input_cb(simio_cb_t cb, void *arg) {
  input_cb.fp = cb;
  input_cb.arg = arg;
}

void sim_io_start() {
  static WORKING_AREA(wsp, THD_SIZE);
  chSchWakeupS(etp = chThdCreateI(wsp, sizeof wsp, NORMALPRIO, event_thread, NULL), RDY_OK);
}

void sim_io_stop() {
  etp->p_flags |= THD_TERMINATE;
}

void sim_getopt(int argc, char **argv) {
  int opt;

  while ((opt = getopt(argc, argv, "i:o:")) != -1) {
    switch (opt) {
      case 'i':
        sdcfg.sd1_port = atoi(optarg);
        break;
      case 'o':
        sdcfg.sd2_port = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                        argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  /* reset getopt */
  optind = 1;
}

/*
 * Activates the serial driver for network IO.
 */
void sim_sdStart() {
  sdStart(&SD1, &sdcfg);
  sdStart(&SD2, &sdcfg);
}

void sim_sdStop() {
  sdStop(&SD1);
  sdStop(&SD2);
}
