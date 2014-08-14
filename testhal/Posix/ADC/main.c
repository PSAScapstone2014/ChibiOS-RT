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

#define DEPTH 1
#define CHANNEL 8

static adcsample_t sample_1[DEPTH * CHANNEL]= {1,2,3,4,5,3,3,5};
static adcsample_t sample_2[4]= {1,2,3,4};
static float out_1[DEPTH * CHANNEL];
static float out_2[4];
/*Adc config driver 1 */
static const ADCConversionGroup adccfg ={
    FALSE,
    CHANNEL,
    NULL,
    NULL, 
    10,
    3,
    out_2,
    4
};
static const ADCConversionGroup adccfg_1 ={
    TRUE,
    CHANNEL,
    NULL,
    NULL,
    10,
    3,
    out_1,
    8
};
/*LED blinker thread*/
static WORKING_AREA(dummy_led, 128);
static msg_t dummy_Thread1(void *arg){
    (void)arg;
    chRegSetThreadName("blinker");
    while(TRUE){
        palSetPad(GPIOD, GPIOD_LED5);
        chThdSleepMilliseconds(500);
        palClearPad(GPIOD, GPIOD_LED5);
        chThdSleepMilliseconds(500);

    }
    return -1;
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
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1) | PAL_PORT_BIT(2), 0, 5);
  chThdCreateStatic(dummy_led, sizeof(dummy_led), NORMALPRIO, dummy_Thread1, NULL);
 //Starting the adc driver
  adcStart(&ADCD1, NULL);
  //single buffer

  adcStartConversion(&ADCD1, &adccfg,sample_2,DEPTH);

  chThdSleepMilliseconds(1000);
  //circular buffer
  adcStartConversion(&ADCD1, &adccfg_1,sample_1,DEPTH);
 
  while (TRUE) {
     if(palReadPad(GPIOA, 0)){
          adcStopConversion(&ADCD1);

     }
    chThdSleepMilliseconds(500);
  }
}
