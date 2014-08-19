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

#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "simio.h"
#define rx_DEPTH  2
#define tx_DEPTH  4

static i2cflags_t errors = 0;
/*static void led4off(void *arg) {

  (void)arg;

  // palClearPad(GPIOC, GPIOC_LED4);
  printf("led4 OFF\n");

}*/



/* Triggered when the button is pressed or released. The LED4 is set to ON.*/
/*static void extcb1(EXTDriver *extp, expchannel_t channel) {
  static VirtualTimer vt4;

  (void)extp;
  (void)channel;

  // palSetPad(GPIOC, GPIOC_LED4);
  printf("led4 ON\n");

  chSysLockFromIsr();
  if (chVTIsArmedI(&vt4))
    chVTResetI(&vt4);*/
  /* LED4 set to OFF after 200mS.*/
/*  chVTSetI(&vt4, MS2ST(200), led4off, NULL);
  chSysUnlockFromIsr();
}*/




static const I2CConfig i2cfg1 ={
    OPMODE_I2C,
    400000,
    FAST_DUTY_CYCLE_2,
};


//Send data
static WORKING_AREA(dummy_1, 256);
static msg_t dummy_1_thread(void *arg)
{
    chRegSetThreadName("dummy1");
    (void)arg;
    uint8_t rx_buf[rx_DEPTH];
    uint8_t tx_buf[tx_DEPTH];
    while(TRUE){
        systime_t tmo = MS2ST(4);

        chThdSleepMilliseconds(32);
        msg_t status = RDY_OK;
        i2cAcquireBus(&I2CD1);
        status = i2cMasterTransmitTimeout(&I2CD1, 0, rx_buf, rx_DEPTH, tx_buf
                ,tx_DEPTH, tmo);
        i2cReleaseBus(&I2CD1);
        if(status != RDY_OK)
        {
            errors = i2cGetErrors(&I2CD1);           
        }
    }
    return 0;
}

//transmit data
static WORKING_AREA(dummy_2, 256);
static msg_t dummy_2_thread(void *arg)
{
    chRegSetThreadName("dummy2");
    (void)arg;
    uint8_t tx_buf[2];
    while(TRUE){
        chThdSleepMilliseconds(32); 
        msg_t status = RDY_OK;
        systime_t tmo = MS2ST(2);
        i2cAcquireBus(&I2CD1);
        status = i2cMasterReceiveTimeout(&I2CD1, 0, tx_buf, 2, tmo);
        i2cReleaseBus(&I2CD1);
        if(status != RDY_OK){
            errors = i2cGetErrors(&I2CD1);
        }
        printf("\n"); 
    }
    return 0;
}


/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();
  chThdSleepMilliseconds(200);
  i2cInit();
  i2cStart(&I2CD1, &i2cfg1);
  palSetPadMode(IOPORT2, 6, 17);
  chThdSleepMilliseconds(100);
  chThdCreateStatic(dummy_1, sizeof(dummy_1), NORMALPRIO, dummy_1_thread, NULL);
  chThdCreateStatic(dummy_2, sizeof(dummy_2), NORMALPRIO, dummy_2_thread, NULL);
  while (TRUE) {
    chThdSleepMilliseconds(5000);
  }
}
