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

#define PWM_INIT 4000
#define PWM_WIDTH 40000
static PWMConfig pwmcfg ={ 
    10000,
    PWM_INIT,
    NULL,
    {
            {PWM_OUTPUT_ACTIVE_HIGH, NULL},
            {PWM_OUTPUT_DISABLED, NULL},
            {PWM_OUTPUT_DISABLED, NULL},
            {PWM_OUTPUT_DISABLED, NULL}
    }
};
static void print_state()
{
    printf("PWM Width: %d \r\n", PWMD1.sim->CCR[0]);
    printf("PWM State: %d \r\n", PWMD1.state);
    printf("PWM Clock: %d \r\n", PWMD1.clock);
    printf("PWM Period: %d \r\n",PWMD1.period);
    printf("PWM Frequency: %d \r\n", PWMD1.config->frequency);
    printf("PWM psc: %d \r\n\n", (PWMD1.clock/PWMD1.config->frequency)-1);

}
static WORKING_AREA(dummy_1, 128);
static msg_t dummy_1_thread(void *arg)
{
    int pwm_period = PWM_INIT;
    bool change = false;
    chRegSetThreadName("pwm_test");
    while(TRUE)
    {
        if(change) {
            pwm_period += 1;
        }
        else
        {
            pwm_period -=1;
        }
       if(pwm_period > (PWM_INIT *0.5)) {
           change = false;
       }
       if(pwm_period < (PWM_WIDTH +1))
       {
           change = true;
       }
       pwmChangePeriod(&PWMD1, pwm_period);
       chThdSleepMilliseconds(200);
       print_state();

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
    int pwm_period = PWM_INIT;
    bool change = false;
  halInit();
  chSysInit();
  palSetPadMode(GPIOD,15, 1);
  pwmStart(&PWMD1,&pwmcfg);
  pwm_lld_enable_channel(&PWMD1, 0, PWM_WIDTH);
  //seg faults..
//  chThdCreateStatic(dummy_1, sizeof(dummy_1), dummy_1_thread,NORMALPRIO ,NULL);
  chThdSleepMilliseconds(200);
  while (TRUE) {
        if(change) {
            pwm_period += 1;
        }
        else
        {
            pwm_period -=1;
        }
       if(pwm_period > (PWM_INIT *0.5)) {
           change = false;
       }
       if(pwm_period < (PWM_WIDTH +1))
       {
           change = true;
       }
       pwmChangePeriod(&PWMD1, pwm_period);
       chThdSleepMilliseconds(200);
       print_state();
    chThdSleepMilliseconds(5000);
  }
}
