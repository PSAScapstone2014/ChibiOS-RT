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
 * @file    Posix/gpt_lld.c
 * @brief   GPT Driver subsystem low level driver source.
 *
 * @addtogroup GPT
 * @{
 */

#include "ch.h"
#include "hal.h"

#if HAL_USE_GPT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*Linux real time clock ID and signal*/
#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   GPTD1 driver identifier.
 */
#if POSIX_GPT_USE_GPT1 || defined(__DOXYGEN__)
  GPTDriver GPTD1;
#endif

/**
 * @brief   GPTD2 driver identifier.
 */
#if POSIX_GPT_USE_GPT2 || defined(__DOXYGEN__)
  GPTDriver GPTD2;
#endif

/*===========================================================================*/
/* Driver local variables and types                                          */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*
* Error handling macro.
*/ 
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)
/* 
* This handler function deals with real time event signals that occur. 
*/
static void handler(int sig, siginfo_t *si, void *uc) {
  printf("Caught signal %d\n", sig);
  signal(sig, SIG_IGN);
}

/*
* This function establishes the declared handler for the signals. 
*/
void establish_sighandler(GPTDriver *gptp) {
  printf("Establishing handler for signal %d\n", SIG);
  gptp->sa.sa_flags = SA_SIGINFO;
  gptp->sa.sa_sigaction = handler;
  sigemptyset(&(gptp->sa.sa_mask));
  if (sigaction(SIG, &(gptp->sa), NULL) == -1)
    errExit("sigaction");
}

/*
* This function is used to block signals for safe handling.
*/
void block(GPTDriver *gptp) {
  printf("Blocking signal %d\n", SIG);
  sigemptyset(&(gptp->mask));
  sigaddset(&(gptp->mask), SIG);
  if (sigprocmask(SIG_BLOCK, &(gptp->mask), NULL) == -1)
    errExit("sigprocmask");
}

/*
* This function is used to unblock signals for safe handling. 
*/ 
void unblock(GPTDriver *gptp) {
  printf("Unblocking signal %d\n", SIG);
  if (sigprocmask(SIG_UNBLOCK, &(gptp->mask), NULL) == -1)
     errExit("sigprocmask");
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level GPT driver initialization.
 * @note    This function's high level  counter part is implicitly invoked by 
 *          @p halInit(), there is no need to explicitly initialize the driver.
 * @notapi
 */
void gpt_lld_init(void) {

#if POSIX_GPT_USE_GPT1
  /* Driver initialization.*/
  gptObjectInit(&GPTD1);
#endif
#if POSIX_GPT_USE_GPT2
  /* Driver initialization.*/
  gptObjectInit(&GPTD2);
#endif
}

/**
 * @brief   Configures and activates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_start(GPTDriver *gptp) {
  
  if (gptp->state == GPT_STOP) {
    /* Enables the peripheral.*/
    /* Configures the peripheral.*/
    block(gptp);
    establish_sighandler(gptp);
    gptp->sev.sigev_notify = SIGEV_SIGNAL;
    gptp->sev.sigev_signo = SIG;
    gptp->sev.sigev_value.sival_ptr = &(gptp->timerid);
    if (timer_create(CLOCKID, &(gptp->sev), &(gptp->timerid)) == -1) 
      errExit("timer_create");
  }
}
/**
 * @brief   Deactivates the GPT peripheral.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop(GPTDriver *gptp) {
  
  if (gptp->state == GPT_READY) {
    /* Resets the peripheral.*/
    /* Disables the peripheral.*/
      timer_delete((&gptp->timerid));
  }
}

/**
 * @brief   Starts the timer in continuous mode.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  period in ticks
 *
 * @notapi
 */
void gpt_lld_start_timer(GPTDriver *gptp, gptcnt_t interval) {

  unblock(gptp);
  establish_sighandler(gptp);
  gptp->sev.sigev_notify = SIGEV_SIGNAL;
  gptp->sev.sigev_signo = SIG;
  gptp->sev.sigev_value.sival_ptr = &(gptp->timerid);
  gptp->freq_nanosecs = (double)interval;
  printf("freq_nanosecs: %d\n", interval);
  gptp->its.it_value.tv_sec = ((double)interval) / (double)CH_FREQUENCY;
  printf("tv_sec: %d\n", gptp->its.it_value.tv_sec);
  gptp->its.it_value.tv_nsec = ((double)interval / CH_FREQUENCY) * 100000000;
  printf("tv_nsec: %d\n", gptp->its.it_value.tv_nsec);
  gptp->its.it_interval.tv_sec = gptp->its.it_value.tv_sec;
  gptp->its.it_interval.tv_nsec = gptp->its.it_value.tv_nsec;
  
  if (timer_settime(gptp->timerid, 0, &(gptp->its), NULL) == -1)
    errExit("timer_settime: start_timer");
}

/**
 * @brief   Stops the timer.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 *
 * @notapi
 */
void gpt_lld_stop_timer(GPTDriver *gptp) {

  (void)gptp;
  block(gptp);
  gptp->freq_nanosecs = 0;
  gptp->its.it_value.tv_sec = 0;
  gptp->its.it_value.tv_nsec = 0;
  gptp->its.it_interval.tv_sec = gptp->its.it_value.tv_sec;
  gptp->its.it_interval.tv_nsec = gptp->its.it_value.tv_nsec;
  
  if (timer_settime(gptp->timerid, 0, &(gptp->its), NULL) == -1)
    errExit("timer_settime: stop_timer");
}

/**
 * @brief   Starts the timer in one shot mode and waits for completion.
 * @details This function specifically polls the timer waiting for completion
 *          in order to not have extra delays caused by interrupt servicing,
 *          this function is only recommended for short delays.
 *
 * @param[in] gptp      pointer to the @p GPTDriver object
 * @param[in] interval  time interval in ticks
 *
 * @notapi
 */
void gpt_lld_polled_delay(GPTDriver *gptp, gptcnt_t interval) {

  (void)gptp;
  (void)interval;
  block(gptp);
  chThdSleepMilliseconds(interval);
  unblock(gptp);
}

#endif /* HAL_USE_GPT */

/** @} */
