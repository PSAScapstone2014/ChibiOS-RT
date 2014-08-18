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
 * @file    templates/i2c_lld.c
 * @brief   I2C Driver subsystem low level driver source template.
 *
 * @addtogroup I2C
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "simio.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if HAL_USE_I2C || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   I2C1 driver identifier.
 */
#if PLATFORM_I2C_USE_I2C1 || defined(__DOXYGEN__)
I2CDriver I2CD1;
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/
 buffer_stream bs_1;
/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void recieve_stream(uint8_t * rxbuf, size_t rxbyte)
{

//   fgets(rxbuf, rxbyte,stdin);
//
   sim_read(SPI_IO, rxbuf, rxbyte);
   
   printf("Recieving %d\n", rxbuf);

}

static void transmit_stream (const uint8_t *txbuf, size_t txbyte)
{
    printf("transmitting %d \n",txbuf);
    sim_write(SPI_IO, txbuf,txbyte);

}


static void i2c_lld_safety_timeout(void *p)
{
    I2CDriver *i2cp = (I2CDriver *)p;
    chSysLockFromIsr();
    if(i2cp->thread){
        Thread *tp = i2cp->thread;
        i2cp->thread = NULL;
        tp->p_u.rdymsg = RDY_TIMEOUT;
        chSchReadyI(tp);
    }
        chSysUnlockFromIsr();
}
/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/
/**
 * @brief   Low level I2C driver initialization.
 *
 * @notapi
 */
void i2c_lld_init(void) {

#if PLATFORM_I2C_USE_I2C1
  i2cObjectInit(&I2CD1);
#endif /* PLATFORM_I2C_USE_I2C1 */
}

/**
 * @brief   Configures and activates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_start(I2CDriver *i2cp) {

  if (i2cp->state == I2C_STOP) {
    /* Enables the peripheral.*/
#if PLATFORM_I2C_USE_I2C1
     if (&I2CD1 == i2cp) {
       
    }
#endif /* PLATFORM_I2C_USE_I2C1 */
  }
  /* Configures the peripheral.*/

}

/**
 * @brief   Deactivates the I2C peripheral.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 *
 * @notapi
 */
void i2c_lld_stop(I2CDriver *i2cp) {

  if (i2cp->state != I2C_STOP) {
    /* Resets the peripheral.*/
    /* Disables the peripheral.*/
#if PLATFORM_I2C_USE_I2C1
    if (&I2CD1 == i2cp) {

    }
#endif /* PLATFORM_I2C_USE_I2C1 */
  }
}

/**
 * @brief   Receives data via the I2C bus as master.
 * @details Number of receiving bytes must be more than 1 on STM32F1x. This is
 *          hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval RDY_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2c_lld_master_receive_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                     uint8_t *rxbuf, size_t rxbytes,
                                     systime_t timeout) {

   VirtualTimer vt;
   chDbgCheck((rxbytes > 1), "i2c_lld_master_recive_timeout");

   if(timeout != TIME_INFINITE)
   {
       chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);
   }
   chSysUnlock();

   i2cp->addr = addr << 1;
   i2cp->errors = 0;

   recieve_stream(rxbuf, rxbytes);



   i2cp->thread = chThdSelf();
   chSchGoSleepS(THD_STATE_SUSPENDED);

  return RDY_TIMEOUT;
  
}

/**
 * @brief   Transmits data via the I2C bus as master.
 * @details Number of receiving bytes must be 0 or more than 1 on STM32F1x.
 *          This is hardware restriction.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   number of bytes to be received
 * @param[in] timeout   the number of ticks before the operation timeouts,
 *                      the following special values are allowed:
 *                      - @a TIME_INFINITE no timeout.
 *                      .
 * @return              The operation status.
 * @retval RDY_OK       if the function succeeded.
 * @retval RDY_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval RDY_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2c_lld_master_transmit_timeout(I2CDriver *i2cp, i2caddr_t addr,
                                      const uint8_t *txbuf, size_t txbytes,
                                      uint8_t *rxbuf, size_t rxbytes,
                                      systime_t timeout) {

   VirtualTimer vt;

  //if the timeout set the virtual timer and uses the function i2c_lld 
   if(timeout != TIME_INFINITE)
   {
       chVTSetI(&vt, timeout, i2c_lld_safety_timeout, (void *)i2cp);
   }
   chSysUnlock();

   i2cp->addr = addr << 1;
   i2cp->errors = 0;

   recieve_stream(rxbuf, rxbytes);

   transmit_stream(txbuf, txbytes);

   i2cp->thread = chThdSelf();
   chSchGoSleepS(THD_STATE_SUSPENDED);

  return RDY_TIMEOUT;

}

#endif /* HAL_USE_I2C */

/** @} */
