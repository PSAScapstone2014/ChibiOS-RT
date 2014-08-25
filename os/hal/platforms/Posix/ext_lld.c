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
#include "simio.h"

#if HAL_USE_EXT || defined(__DOXYGEN__)

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
static WORKING_AREA(wsp, 128);
static Thread *rthd;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

static msg_t read_thread(void *arg) {
  EXTDriver *extp = (EXTDriver*)arg;
  uint8_t channel;
  while (TRUE) {
    int nb = sim_read(EXT_IO, &channel, sizeof channel);
    if (nb < 0) {
      /* don't spam errors too quickly */
      chThdSleep(S2ST(1));
    }
    else if (nb > 0) {
      if (channel < EXT_MAX_CHANNELS
          && extp->channelsEnabled[channel]
          && extp->config->channels[channel].cb) {
        CH_IRQ_PROLOGUE();
        extp->config->channels[channel].cb(extp, channel);
        CH_IRQ_EPILOGUE();
      }
    }
  }
  return 0;
}

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
  int i;

  if (extp->state == EXT_STOP) {

    /* Enables the peripheral.*/
    extp->state = EXT_ACTIVE;
    for (i = 0; i < EXT_MAX_CHANNELS; i++)
      extp->channelsEnabled[i] = TRUE;

    rthd = chThdCreateI(wsp, sizeof(wsp), NORMALPRIO, read_thread, (void*)extp);
    chSchWakeupS(rthd, RDY_OK);

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
    sim_disconnect();

    /* Disables the peripheral.*/
    extp->state = EXT_STOP;
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
  extp->channelsEnabled[channel] = TRUE;
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
  extp->channelsEnabled[channel] = FALSE;
}

#endif /* HAL_USE_EXT */

/** @} */
