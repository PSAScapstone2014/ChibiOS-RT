/*
    ChibiOS/RT - Copyright (C) 2014 Nicholas T. Lamkins

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    simio.h
 * @brief   Simulator macros and data structures.
 *
 * @addtogroup SIMIO
 * @{
 */

#ifndef SIMIO_H
#define SIMIO_H

#if defined(SIMULATOR) || defined(__DOXYGEN__)

#include <stdio.h>
#include "ch.h"

/* data buffer size for sim_printf */
#define SIM_PRINTF_MAX 1024

/* maximum pending reads/writes */
#define MB_QUEUE_SIZE 10

/* HAL Identifiers */
typedef enum {
  SIM_IO,   /* Do not initialize the first element to anything other than 0 */
  PAL_IO,
  SD1_IO,
  SD2_IO,
  EXT_IO,
  SPI_IO,
  SDC_IO,
  I2C_IO,
  PWM_IO,
  HID_COUNT /* HID_COUNT must be last */
} hid_t;

typedef struct {
  hid_t     id;
  uint32_t  sid;
} sim_hal_id_t;

/* configure based on command line arguments */
extern void sim_getopt(int argc, char **argv);

/* read data sent to the HAL */
extern ssize_t sim_read_timeout(sim_hal_id_t *hid, void *buf, size_t bufsz, int timeout);
extern ssize_t sim_read_timeoutS(sim_hal_id_t *hid, void *buf, size_t bufsz, int timeout);
#define sim_read(a,b,c) sim_read_timeout((a),(b),(c),TIME_INFINITE)
#define sim_readS(a,b,c) sim_read_timeoutS((a),(b),(c),TIME_INFINITE)

/* write data from the HAL */
extern ssize_t sim_write(sim_hal_id_t *hid, void *buf, size_t bufsz);
extern int sim_printf(sim_hal_id_t *hid, char *fmt, ...);

/* shutdown */
extern int sim_disconnect(void);

#endif /* SIMULATOR */
#endif /* SIMIO_H */

/** @} */
