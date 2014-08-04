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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    STM32/RTCv1/rtc_lld.c
 * @brief   STM32 RTC subsystem low level driver header.
 *
 * @addtogroup RTC
 * @{
 */

#include "ch.h"
#include "hal.h"

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#if HAL_USE_RTC || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief RTC driver identifier.
 */
RTCDriver RTCD1;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/
static WORKING_AREA(wsp, 128);
static Thread *athd;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Load value of RTCCLK to prescaler registers.
 * @note    The pre-scaler must not be set on every reset as RTC clock
 *          counts are lost when it is set.
 * @note    This function designed to be called from
 *          hal_lld_backup_domain_init(). Because there is only place
 *          where possible to detect BKP domain reset event reliably.
 *
 * @notapi
 */
#define rtc_lld_set_prescaler()

/**
 * @brief   Initialize RTC.
 *
 * @notapi
 */
void rtc_lld_init(void) {
}

/**
 * @brief   Set current time.
 * @note    Fractional part will be silently ignored. There is no possibility
 *          to change it on STM32F1xx platform.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @notapi
 */
void rtc_lld_set_time(RTCDriver *rtcp, const RTCTime *timespec) {
  (void)rtcp;
  (void)timespec;
}

/**
 * @brief   Get current time.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] timespec  pointer to a @p RTCTime structure
 *
 * @notapi
 */
void rtc_lld_get_time(RTCDriver *rtcp, RTCTime *timespec) {
  (void)rtcp;
  gettimeofday((struct timeval*)timespec, NULL);
}

/**
 * @brief   Callback for rtc_lld_set_alarm()
 *
 * @notapi
 */
void rtd_lld_alarm_do_callback(int signal) {
  (void)signal;
  if (RTCD1.callback) {
    CH_IRQ_PROLOGUE();
    RTCD1.callback(&RTCD1, RTC_EVENT_ALARM);
    CH_IRQ_EPILOGUE();
  }
}

/**
 * @brief   Signal every second
 *
 * @notapi
 */
static msg_t alarm_thread(void *arg) {
  while (TRUE) {
    chThdSleep(1000);
    if (RTCD1.callback) {
      CH_IRQ_PROLOGUE();
      RTCD1.callback(&RTCD1, RTC_EVENT_SECOND);
      CH_IRQ_EPILOGUE();
    }
  }
}

/**
 * @brief   Set alarm time.
 *
 * @note      Default value after BKP domain reset is 0xFFFFFFFF
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[in] alarmspec pointer to a @p RTCAlarm structure
 *
 * @notapi
 */
void rtc_lld_set_alarm(RTCDriver *rtcp,
                       rtcalarm_t rtcalarm,
                       const RTCAlarm *alarmspec) {
  (void)rtcalarm;

  RTCTime now;
  rtc_lld_get_time(rtcp, &now);

  struct sigaction act;
  act.sa_handler = rtd_lld_alarm_do_callback;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESETHAND;

  sigaction(SIGALRM, &act, NULL);
  alarm(alarmspec->tv_sec - now.tv_sec);
}

/**
 * @brief   Get current alarm.
 * @note    If an alarm has not been set then the returned alarm specification
 *          is not meaningful.
 *
 * @note    Default value after BKP domain reset is 0xFFFFFFFF.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] alarm     alarm identifier
 * @param[out] alarmspec pointer to a @p RTCAlarm structure
 *
 * @notapi
 */
void rtc_lld_get_alarm(RTCDriver *rtcp,
                       rtcalarm_t rtcalarm,
                       RTCAlarm *alarmspec) {

  (void)rtcp;
  (void)rtcalarm;

  alarmspec->tv_sec = alarm(0);
}

/**
 * @brief   Enables or disables RTC callbacks.
 * @details This function enables or disables callbacks, use a @p NULL pointer
 *          in order to disable a callback.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @param[in] callback  callback function pointer or @p NULL
 *
 * @notapi
 */
void rtc_lld_set_callback(RTCDriver *rtcp, rtccb_t callback) {
    rtcp->callback = callback;

    /* spawn thread that signals once per second */
    if (!athd) {
      athd = chThdCreateI(wsp, sizeof(wsp), NORMALPRIO, alarm_thread, NULL);
      chSchWakeupS(athd, RDY_OK);
    }
}

#include "chrtclib.h"

/**
 * @brief   Get current time in format suitable for usage in FatFS.
 *
 * @param[in] rtcp      pointer to RTC driver structure
 * @return              FAT time value.
 *
 * @api
 */
uint32_t rtc_lld_get_time_fat(RTCDriver *rtcp) {
  uint32_t fattime;
  struct tm timp;

  rtcGetTimeTm(rtcp, &timp);

  fattime  = (timp.tm_sec)       >> 1;
  fattime |= (timp.tm_min)       << 5;
  fattime |= (timp.tm_hour)      << 11;
  fattime |= (timp.tm_mday)      << 16;
  fattime |= (timp.tm_mon + 1)   << 21;
  fattime |= (timp.tm_year - 80) << 25;

  return fattime;
}
#endif /* HAL_USE_RTC */

/** @} */
