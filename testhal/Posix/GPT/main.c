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

#include "ch.h"
#include "hal.h"


/*
* GPT2 callback.
*/
static void gpt2cb(GPTDriver *gptp) {

  (void)gptp;
  printf("callback to LED5, timer id:  %x\n", (unsigned int)gptp->timerid);
  palSetPad(GPIOD, GPIOD_LED5);
  printf("Got here1\n");
  chSysLockFromIsr();
  printf("Got here2\n");
  gptStartOneShotI(&GPTD2, 1000); /* 0.1 second pulse.*/
  printf("Got here3\n");
  chSysUnlockFromIsr();
  printf("Got here4\n");
}

/*
* GPT1 callback.
*/
static void gpt1cb(GPTDriver *gptp) {

  (void)gptp;
  printf("Callback to LED5, timer id: %x\n", (unsigned int)gptp->timerid);
  palClearPad(GPIOD, GPIOD_LED5);
}

/*
* GPT1 configuration.
*/
static const GPTConfig gpt1cfg = {
  10000, /* 10kHz timer clock.*/
  gpt1cb, /* Timer callback.*/
};

/*
* GPT2 configuration.
*/
static const GPTConfig gpt2cfg = {
  10000, /* 10kHz timer clock.*/
  gpt2cb /* Timer callback.*/
};

/*
* Application entry point.
*/
int main(void) {

  /*
* System initializations.
* - HAL initialization, this also initializes the configured device drivers
* and performs the board-specific initializations.
* - Kernel initialization, the main() function becomes a thread and the
* RTOS is active.
*/
  halInit();
  chSysInit();

  /*
* Initializes the GPT drivers 2 and 3.
*/
  printf("Start GPTD1\n");
  gptStart(&GPTD1, &gpt1cfg);
  printf("timer ID is 0x%lx\n", (long) GPTD1.timerid);
  printf("Finished gptStartD1\n");
  printf("Going to sleep D1 for: %d \n", 10);
  gptPolledDelay(&GPTD1, 10); /* Small delay.*/
  printf("Finished polledDelay1\n");
  printf("Start GPTD2\n");
  gptStart(&GPTD2, &gpt2cfg);
  printf("timer ID is 0x%lx\n", (long) GPTD2.timerid);
  printf("Going to sleep D2 for: %d \n", 10);
  gptPolledDelay(&GPTD2, 10); /* Small delay.*/
  printf("polledDelay2\n");

  /*
* Normal main() thread activity, it changes the GPT1 period every
* five seconds and the GPT2 period every 2.5 seconds.
*/
  while (TRUE) {
    //palSetPad(GPIOD, GPIOD_LED4);
    printf("Start Continuous: GPTD1\n");
    gptStartContinuous(&GPTD1, 5000);
    printf("Sleep for 5 seconds\n");
    chThdSleepMilliseconds(5000);
    printf("Stopping GPTD1\n");
    gptStopTimer(&GPTD1);
    //palClearPad(GPIOD, GPIOD_LED4);
    printf("Start Continuous: GPTD2\n");
    gptStartContinuous(&GPTD2, 2500);
    printf("Sleep for 2.5 seconds\n");
    chThdSleepMilliseconds(2500);
    printf("Stopping GPTD2\n");
    gptStopTimer(&GPTD2);
  }
}
