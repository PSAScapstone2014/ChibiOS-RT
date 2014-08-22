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
#include "ch.h"
#include "hal.h"

/*
 * Generic SPI configuration.
 */
static const SPIConfig spicfg = {
  NULL,
  GPIOB,
  12,
  0,
  0
};

/*
 * SPI TX and RX buffers.
 */
static uint8_t txbuf[512];
static uint8_t rxbuf[512];

/*
 * SPI bus contender 1.
 */
static WORKING_AREA(spi_thread_1_wa, 256);
static msg_t spi_thread_1(void *p) {

  (void)p;
  chRegSetThreadName("SPI thread 1");
  while (TRUE) {
    spiAcquireBus(&SPID1);              /* Acquire ownership of the bus.    */
    palSetPad(GPIOD, GPIOD_LED5);       /* LED ON.                          */
    spiStart(&SPID1, &spicfg);          /* Setup transfer parameters.       */
    spiSelect(&SPID1);                  /* Slave Select assertion.          */
    spiExchange(&SPID1, 512,
                txbuf, rxbuf);          /* Atomic transfer operations.      */
    spiUnselect(&SPID1);                /* Slave Select de-assertion.       */
    spiReleaseBus(&SPID1);              /* Ownership release.               */
    chThdSleep(S2ST(1));
  }
  return 0;
}

/*
 * SPI bus contender 2.
 */
static WORKING_AREA(spi_thread_2_wa, 256);
static msg_t spi_thread_2(void *p) {

  (void)p;
  chRegSetThreadName("SPI thread 2");
  while (TRUE) {
    spiAcquireBus(&SPID1);              /* Acquire ownership of the bus.    */
    palClearPad(GPIOD, GPIOD_LED5);     /* LED OFF.                         */
    spiStart(&SPID1, &spicfg);          /* Setup transfer parameters.       */
    spiSelect(&SPID1);                  /* Slave Select assertion.          */
    spiExchange(&SPID1, 512,
                txbuf, rxbuf);          /* Atomic transfer operations.      */
    spiUnselect(&SPID1);                /* Slave Select de-assertion.       */
    spiReleaseBus(&SPID1);              /* Ownership release.               */
    chThdSleep(S2ST(1));
  }
  return 0;
}

/*
 * Application entry point.
 */
int main(void) {
  unsigned i;
  uint16_t frame;

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
   * SPI2 I/O pins setup.
   */
  palSetPad(GPIOB, 12);

  /*
   * Prepare transmit pattern.
   */
  for (i = 0; i < sizeof(txbuf); i++)
    txbuf[i] = (uint8_t)i;

  spiAcquireBus(&SPID1);              /* Acquire ownership of the bus.    */
  spiStart(&SPID1, &spicfg);          /* Setup transfer parameters.       */
  spiSelect(&SPID1);                  /* Slave Select assertion.          */

  spiExchange(&SPID1, 512, txbuf, rxbuf);
  spiStartExchange(&SPID1, 512, txbuf, rxbuf);

  spiSend(&SPID1, sizeof txbuf, txbuf);
  spiStartSend(&SPID1, sizeof txbuf, txbuf);

  spiReceive(&SPID1, sizeof rxbuf, rxbuf);
  spiStartReceive(&SPID1, sizeof rxbuf, rxbuf);

  if ((frame = spiPolledExchange(&SPID1, 0xaa)) != 0xaa) {
    fprintf(stderr, "ERROR spiPolledExchange expected 0xaa got 0x%x", frame);
    exit(1);
  }

  spiUnselect(&SPID1);                /* Slave Select de-assertion.       */
  spiReleaseBus(&SPID1);              /* Ownership release.               */

  /*
   * Starting the transmitter and receiver threads.
   */
  chThdCreateStatic(spi_thread_1_wa, sizeof(spi_thread_1_wa),
                    NORMALPRIO + 1, spi_thread_1, NULL);
  chThdCreateStatic(spi_thread_2_wa, sizeof(spi_thread_2_wa),
                    NORMALPRIO + 1, spi_thread_2, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing.
   */
  while (TRUE) {
    chThdSleep(S2ST(1));
  }
  return 0;
}
