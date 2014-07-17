/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    templates/ext_lld.c
 * @brief   EXT Driver subsystem low level driver source template.
 *
 * @addtogroup EXT
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <stdio.h>

#if HAL_USE_EXT || defined(__DOXYGEN__)

#define SHELL_WA_SIZE       THD_WA_SIZE(256)
#define CONSOLE_WA_SIZE     THD_WA_SIZE(256)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   EXT1 driver identifier.
 */
#if PLATFORM_EXT_USE_EXT1 || defined(__DOXYGEN__)
EXTDriver EXTD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

static EventListener sd1fel;
static Thread *shelltp1;

/**
 * @brief   Shell termination event source.
 */
EventSource shell_terminated;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

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
static msg_t shell_thread(void *vptr) {
  BaseSequentialStream *chp = vptr;
  uint8_t c, chan;

  if (chSequentialStreamRead(chp, &c, 1) == 0)
    return 0;

  chan = c - '0';
  if (chan < EXT_MAX_CHANNELS) {
    printf("channel %d mode %d\n", chan, EXTD1.config->channels[chan].mode);
    EXTD1.config->channels[chan].cb(&EXTD1, chan);
  }

  /* Atomically broadcasting the event source and terminating the thread,
     there is not a chSysUnlock() because the thread terminates upon return.*/
  chSysLock();
  chEvtBroadcastI(&shell_terminated);
  chThdExitS(RDY_OK);

  /* Never executed, silencing a warning.*/
  return 0;
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
    // printf("Init: connection on SD1\n");
    shelltp1 = chThdCreateFromHeap(NULL, SHELL_WA_SIZE, NORMALPRIO + 10, shell_thread, (void *)&SD1);
  }
  if (flags & CHN_DISCONNECTED) {
    // printf("Init: disconnection on SD1\n");
    chSysLock();
    chIQResetI(&SD1.iqueue);
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
}

static evhandler_t fhandlers[] = {
  termination_handler,
  sd1_handler
};

static void event_thread(void *arg) {
  (void)arg;
  EventListener tel;

  chEvtInit(&shell_terminated);

  chEvtRegister(&shell_terminated, &tel, 0);
  chEvtRegister(chnGetEventSource(&SD1), &sd1fel, 1);

  /*
   * Events servicing loop.
   */
  while (!chThdShouldTerminate())
    chEvtDispatch(fhandlers, chEvtWaitOne(ALL_EVENTS));

  /*
   * Clean simulator exit.
   */
  chEvtUnregister(chnGetEventSource(&SD1), &sd1fel);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level EXT driver initialization.
 *
 * @notapi
 */
void ext_lld_init(void) {

#if PLATFORM_EXT_USE_EXT1
  /* Driver initialization.*/
  extObjectInit(&EXTD1);

#endif /* PLATFORM_EXT_USE_EXT1 */
}

/**
 * @brief   Configures and activates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @notapi
 */
void ext_lld_start(EXTDriver *extp) {

  if (extp->state == EXT_STOP) {
    /* Enables the peripheral.*/
#if PLATFORM_EXT_USE_EXT1
    if (&EXTD1 == extp) {
    }
#endif /* PLATFORM_EXT_USE_EXT1 */
  }
  /* Configures the peripheral.*/

  sdStart(&SD1, NULL);

  (void)chThdCreateFromHeap(NULL, CONSOLE_WA_SIZE, NORMALPRIO,
                            event_thread, NULL);

}

/**
 * @brief   Deactivates the EXT peripheral.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 *
 * @notapi
 */
void ext_lld_stop(EXTDriver *extp) {

  if (extp->state == EXT_ACTIVE) {
    /* Resets the peripheral.*/

    /* Disables the peripheral.*/
#if PLATFORM_EXT_USE_EXT1
    if (&EXTD1 == extp) {

    }
#endif /* PLATFORM_EXT_USE_EXT1 */
  }
}

/**
 * @brief   Enables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be enabled
 *
 * @notapi
 */
void ext_lld_channel_enable(EXTDriver *extp, expchannel_t channel) {

  (void)channel;

#if PLATFORM_EXT_USE_EXT1
    if (&EXTD1 == extp) {

    }
#endif /* PLATFORM_EXT_USE_EXT1 */

}

/**
 * @brief   Disables an EXT channel.
 *
 * @param[in] extp      pointer to the @p EXTDriver object
 * @param[in] channel   channel to be disabled
 *
 * @notapi
 */
void ext_lld_channel_disable(EXTDriver *extp, expchannel_t channel) {

  (void)extp;
  (void)channel;

}

#endif /* HAL_USE_EXT */

/** @} */
