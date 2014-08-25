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

#ifndef CH_DEMO

/**
 * @file    Posix/serial_lld.c
 * @brief   Posix low level simulated serial driver code.
 *
 * @addtogroup POSIX_SERIAL
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "ch.h"
#include "hal.h"
#include "sim_preempt.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief Serial driver 1 identifier.*/
#if USE_SIM_SERIAL1 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif
/** @brief Serial driver 2 identifier.*/
#if USE_SIM_SERIAL2 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const SerialConfig default_config = {
};

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
 * @brief   Low level serial driver initialization.
 */
void sd_lld_init(void) {

#if USE_SIM_SERIAL1
  sdObjectInit(&SD1, NULL, NULL);

  SD1.hid = SD1_IO;

#endif /* USE_SIM_SERIAL1 */

#if USE_SIM_SERIAL2
  sdObjectInit(&SD2, NULL, NULL);

  SD2.hid = SD2_IO;

#endif /* USE_SIM_SERIAL2 */
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {
  if (config == NULL)
    config = &default_config;
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
void sd_lld_stop(SerialDriver *sdp) {
  (void)sdp;
}

ssize_t _serial_lld_read(SerialDriver *sdp, uint8_t *b, size_t n) {
  return sim_read(sdp->hid, b, n);
}

ssize_t _serial_lld_read_timeout(SerialDriver *sdp, uint8_t *b, size_t n, systime_t t) {
  return sim_read_timeout(sdp->hid, b, n, t);
}

ssize_t _serial_lld_write(SerialDriver *sdp, uint8_t *b, size_t n) {
  return sim_write(sdp->hid, b, n);
}

bool_t sd_lld_interrupt_pending(void) {
  return FALSE;
}

#endif /* HAL_USE_SERIAL */

/** @} */

#endif /* CH_DEMO */
