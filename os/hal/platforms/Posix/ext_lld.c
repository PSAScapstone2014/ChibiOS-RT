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

#include <stdlib.h>
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "simio.h"

#if HAL_USE_EXT || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
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
static void ext_input_handler(char *buf, void *vptr) {
  uint32_t channel;
  EXTDriver *extp = vptr;

  channel = atoi(buf);
  if (channel < EXT_MAX_CHANNELS && extp->config->channels[channel].cb) {
    dbg_check_enter_isr();
    extp->config->channels[channel].cb(extp, channel);
    dbg_check_leave_isr();
  }
}


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
      sim_set_input_cb(HAL_EXT, ext_input_handler, (void*)extp);
    }
#endif /* PLATFORM_EXT_USE_EXT1 */
  }

  /* Configures the peripheral.*/
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
      sim_io_stop();
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
