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
#include <string.h>
#include "ch.h"
#include "hal.h"

#ifdef MMCSD_BLOCK_SIZE /* this hack may need to be accounted for in the posix lld */
#undef MMCSD_BLOCK_SIZE
#define MMCSD_BLOCK_SIZE 1
#endif

#define SDC_DATA_DESTRUCTIVE_TEST   TRUE

#define SDC_BURST_SIZE      8 /* how many sectors reads at once */
static uint8_t outbuf[MMCSD_BLOCK_SIZE * SDC_BURST_SIZE + 1];
static uint8_t  inbuf[MMCSD_BLOCK_SIZE * SDC_BURST_SIZE + 1];

/**
 * @brief   Returns the card insertion status.
 * @note    this function must be provided by the application because
 *          it is not part of the SDC driver.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @return              The card state.
 * @retval FALSE        card not inserted.
 * @retval TRUE         card inserted.
 *
 */
bool_t sdc_lld_is_write_protected(SDCDriver *sdcp) {
  (void)sdcp;
  return FALSE;
}

/**
 * @brief   Returns the write protect status.
 * @note    This function must be provided by the application because
 *          it is not part of the SDC driver.
 *
 * @param[in] sdcp      pointer to the @p SDCDriver object
 * @return              The card state.
 * @retval FALSE        not write protected.
 * @retval TRUE         write protected.
 *
 */
bool_t sdc_lld_is_card_inserted(SDCDriver *sdcp) {
  (void)sdcp;
  return !palReadPad(GPIOC, GPIOC_PIN0);
}

/**
 * @brief   Parody of UNIX badblocks program.
 *
 * @param[in] start       first block to check
 * @param[in] end         last block to check
 * @param[in] blockatonce number of blocks to check at once
 * @param[in] pattern     check pattern
 *
 * @return              The operation status.
 * @retval SDC_SUCCESS  operation succeeded, the requested blocks have been
 *                      read.
 * @retval SDC_FAILED   operation failed, the state of the buffer is uncertain.
 */
bool_t badblocks(uint32_t start, uint32_t end, uint32_t blockatonce, uint8_t pattern){
  uint32_t position = 0;
  uint32_t i = 0;

  chDbgCheck(blockatonce <= SDC_BURST_SIZE, "badblocks");

  /* fill control buffer */
  for (i=0; i < MMCSD_BLOCK_SIZE * blockatonce; i++)
    outbuf[i] = pattern;

  /* fill SD card with pattern. */
  position = start;
  while (position < end){
    if (sdcWrite(&SDCD1, position, outbuf, blockatonce))
      goto ERROR;
    position += blockatonce;
  }

  /* read and compare. */
  position = start;
  while (position < end){
    if (sdcRead(&SDCD1, position, inbuf, blockatonce))
      goto ERROR;
    if (memcmp(inbuf, outbuf, blockatonce * MMCSD_BLOCK_SIZE) != 0)
      goto ERROR;
    position += blockatonce;
  }
  return FALSE;

ERROR:
  return TRUE;
}

/**
 *
 */
void fillbuffer(uint8_t pattern, uint8_t *b){
  uint32_t i = 0;
  for (i=0; i < MMCSD_BLOCK_SIZE * SDC_BURST_SIZE; i++)
    b[i] = pattern;
}

/**
 *
 */
void fillbuffers(uint8_t pattern){
  fillbuffer(pattern, inbuf);
  fillbuffer(pattern, outbuf);
}

/**
 *
 */
void sdiotest(void){
  uint32_t i = 0;

  printf("Trying to connect SDIO... ");
  chThdSleepMilliseconds(100);

  if (!sdcConnect(&SDCD1)) {

    printf("OK\r\n");
    printf("*** Card CSD content is: ");
    printf("%X %X %X %X \r\n", (&SDCD1)->csd[3], (&SDCD1)->csd[2],
                                      (&SDCD1)->csd[1], (&SDCD1)->csd[0]);

    printf("Single aligned read...");
    chThdSleepMilliseconds(100);
    if (sdcRead(&SDCD1, 0, inbuf, 1))
      chSysHalt();
    printf(" OK\r\n");
    chThdSleepMilliseconds(100);


    printf("Single unaligned read...");
    chThdSleepMilliseconds(100);
    if (sdcRead(&SDCD1, 0, inbuf + 1, 1))
      chSysHalt();
    if (sdcRead(&SDCD1, 0, inbuf + 2, 1))
      chSysHalt();
    if (sdcRead(&SDCD1, 0, inbuf + 3, 1))
      chSysHalt();
    printf(" OK\r\n");
    chThdSleepMilliseconds(100);


    printf("Multiple aligned reads...");
    chThdSleepMilliseconds(100);
    fillbuffers(0x55);
    /* fill reference buffer from SD card */
    if (sdcRead(&SDCD1, 0, inbuf, SDC_BURST_SIZE))
      chSysHalt();
    for (i=0; i<1000; i++){
      if (sdcRead(&SDCD1, 0, outbuf, SDC_BURST_SIZE))
        chSysHalt();
      if (memcmp(inbuf, outbuf, SDC_BURST_SIZE * MMCSD_BLOCK_SIZE) != 0)
        chSysHalt();
    }
    printf(" OK\r\n");
    chThdSleepMilliseconds(100);


    printf("Multiple unaligned reads...");
    chThdSleepMilliseconds(100);
    fillbuffers(0x55);
    /* fill reference buffer from SD card */
    if (sdcRead(&SDCD1, 0, inbuf + 1, SDC_BURST_SIZE))
      chSysHalt();
    for (i=0; i<1000; i++){
      if (sdcRead(&SDCD1, 0, outbuf + 1, SDC_BURST_SIZE))
        chSysHalt();
      if (memcmp(inbuf, outbuf, SDC_BURST_SIZE * MMCSD_BLOCK_SIZE) != 0)
        chSysHalt();
    }
    printf(" OK\r\n");
    chThdSleepMilliseconds(100);

#if SDC_DATA_DESTRUCTIVE_TEST

    printf("Single aligned write...");
    chThdSleepMilliseconds(100);
    fillbuffer(0xAA, inbuf);
    if (sdcWrite(&SDCD1, 0, inbuf, 1))
      chSysHalt();
    fillbuffer(0, outbuf);
    if (sdcRead(&SDCD1, 0, outbuf, 1))
      chSysHalt();
    if (memcmp(inbuf, outbuf, MMCSD_BLOCK_SIZE) != 0)
      chSysHalt();
    printf(" OK\r\n");

    printf("Single unaligned write...");
    chThdSleepMilliseconds(100);
    fillbuffer(0xFF, inbuf);
    if (sdcWrite(&SDCD1, 0, inbuf+1, 1))
      chSysHalt();
    fillbuffer(0, outbuf);
    if (sdcRead(&SDCD1, 0, outbuf+1, 1))
      chSysHalt();
    if (memcmp(inbuf+1, outbuf+1, MMCSD_BLOCK_SIZE) != 0)
      chSysHalt();
    printf(" OK\r\n");

    printf("Running badblocks at 0x10000 offset...");
    chThdSleepMilliseconds(100);
    if(badblocks(0x10000, 0x11000, SDC_BURST_SIZE, 0xAA))
      chSysHalt();
    printf(" OK\r\n");
#endif /* !SDC_DATA_DESTRUCTIVE_TEST */

  }
  else{
    chSysHalt();
  }
}


/*
 * SDIO configuration.
 */
static const SDCConfig sdccfg = {
  0
};

/*
 * Application entry point.
 */
int main(void) {
  halInit();
  chSysInit();

  /*
   * Initializes the SDIO drivers.
   */
  sdcStart(&SDCD1, &sdccfg);

  sdiotest();

  return 0;
}
