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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "simio.h"

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

SerialDriver SD1;

/*
 * Application entry point.
 */
int main(int argc, char **argv) {
  char buf[256];
  ssize_t nb;

  /* no stdout buffering */
  setbuf(stdout, NULL);

  /* send args to simulator */
  sim_getopt(argc, argv);

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the EXT driver 1.
   */
  sdStart(&SD1, NULL);

  while (TRUE) {
    while ((nb = sdRead(&SD1, (uint8_t*)buf, sizeof buf)) <= 0) {
      chThdSleep(1000);
    }

    /* echo the data back */
    (void)sdWrite(&SD1, (uint8_t*)buf, nb);
  }

  return 0;
}
