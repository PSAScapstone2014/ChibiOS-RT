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
 * @file    Posix/pal_lld.c
 * @brief   Posix low level simulated PAL driver code.
 *
 * @addtogroup POSIX_PAL
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "simio.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/**
 * @brief   VIO1 simulated port.
 */
sim_vio_port_t vio_port_1;

/**
 * @brief   VIO2 simulated port.
 */
sim_vio_port_t vio_port_2;
/**
 * @brief   VIO1 simulated port.
 */
sim_vio_port_t vio_port_3;

/**
 * @brief   VIO2 simulated port.
 */
sim_vio_port_t vio_port_4;

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

char* port_lookup(ioportid_t port) {
  if (port == IOPORT1)
    return "IOPORT1";
  else if (port == IOPORT2)
    return "IOPORT2";
  else if (port == IOPORT3)
    return "IOPORT3";
  else if (port == IOPORT4)
    return "IOPORT4";
  else
    return "IOPORT_UNKNOWN";
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void _pal_lld_init(const PALConfig *config) {
  vio_port_1 = (config)->VP1Data;
  vio_port_2 = (config)->VP2Data;
  vio_port_3 = (config)->VP3Data;
  vio_port_4 = (config)->VP4Data;
}

/**
 * @brief   Writes a bits mask on a I/O port.
 *
 * @param[in] port      port identifier
 * @param[in] bits      bits to be written on the specified port
 *
 * @notapi
 */
 void _pal_lld_writeport(ioportid_t port, uint32_t bits) {
  sim_printf(PAL_IO, "set %s latch to 0x%02x (was 0x%02x)\n", port_lookup(port), bits, port->latch);
  port->latch = bits;
}

/**
 * @brief Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 *
 * @param[in] port the port identifier
 * @param[in] mask the group mask
 * @param[in] mode the mode
 *
 * @note This function is not meant to be invoked directly by the application
 *       code.
 * @note @p PAL_MODE_UNCONNECTED is implemented as push pull output with high
 *       state.
 * @note This function does not alter the @p PINSELx registers. Alternate
 *       functions setup must be handled by device-specific code.
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

  switch (mode) {
  case PAL_MODE_RESET:
  case PAL_MODE_INPUT:
    port->dir &= ~mask;
    break;
  case PAL_MODE_UNCONNECTED:
    port->latch |= mask;
  case PAL_MODE_OUTPUT_PUSHPULL:
    port->dir |= mask;
    break;
  }
}

#endif /* HAL_USE_PAL */

/** @} */
