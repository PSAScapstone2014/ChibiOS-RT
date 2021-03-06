/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

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
 * @file    serial.h
 * @brief   Serial Driver macros and structures.
 *
 * @addtogroup SERIAL
 * @{
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    Serial status flags
 * @{
 */
#define SD_PARITY_ERROR         32  /**< @brief Parity error happened.      */
#define SD_FRAMING_ERROR        64  /**< @brief Framing error happened.     */
#define SD_OVERRUN_ERROR        128 /**< @brief Overflow happened.          */
#define SD_NOISE_ERROR          256 /**< @brief Noise on the line.          */
#define SD_BREAK_DETECTED       512 /**< @brief Break detected.             */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Serial configuration options
 * @{
 */
/**
 * @brief   Default bit rate.
 * @details Configuration parameter, this is the baud rate selected for the
 *          default configuration.
 */
#if !defined(SERIAL_DEFAULT_BITRATE) || defined(__DOXYGEN__)
#define SERIAL_DEFAULT_BITRATE      38400
#endif

/**
 * @brief   Serial buffers size.
 * @details Configuration parameter, you can change the depth of the queue
 *          buffers depending on the requirements of your application.
 * @note    The default is 16 bytes for both the transmission and receive
 *          buffers.
 */
#if !defined(SERIAL_BUFFERS_SIZE) || defined(__DOXYGEN__)
#define SERIAL_BUFFERS_SIZE         16
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !CH_USE_QUEUES && !CH_USE_EVENTS
#error "Serial Driver requires CH_USE_QUEUES and CH_USE_EVENTS"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Driver state machine possible states.
 */
typedef enum {
  SD_UNINIT = 0,                    /**< Not initialized.                   */
  SD_STOP = 1,                      /**< Stopped.                           */
  SD_READY = 2                      /**< Ready.                             */
} sdstate_t;

/**
 * @brief   Structure representing a serial driver.
 */
typedef struct SerialDriver SerialDriver;

#include "serial_lld.h"

/**
 * @brief   @p SerialDriver specific methods.
 */
#define _serial_driver_methods                                              \
  _base_asynchronous_channel_methods

/**
 * @extends BaseAsynchronousChannelVMT
 *
 * @brief   @p SerialDriver virtual methods table.
 */
struct SerialDriverVMT {
  _serial_driver_methods
};

/**
 * @extends BaseAsynchronousChannel
 *
 * @brief   Full duplex serial driver class.
 * @details This class extends @p BaseAsynchronousChannel by adding physical
 *          I/O queues.
 */
struct SerialDriver {
  /** @brief Virtual Methods Table.*/
  const struct SerialDriverVMT *vmt;
  _serial_driver_data
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Direct output check on a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          checks directly the output queue. This is faster but cannot
 *          be used to check different channels implementations.
 *
 * @deprecated
 *
 * @api
 */
#if !defined(serial_lld_put_would_block) || defined(__DOXYGEN__)
#define sdPutWouldBlock(sdp) chOQIsFullI(&(sdp)->oqueue)
#else
#define sdPutWouldBlock(sdp) serial_lld_put_would_block((sdp))
#endif

/**
 * @brief   Direct input check on a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          checks directly the input queue. This is faster but cannot
 *          be used to check different channels implementations.
 *
 * @deprecated
 *
 * @api
 */
#if !defined(serial_lld_get_would_block) || defined(__DOXYGEN__)
#define sdGetWouldBlock(sdp) chIQIsEmptyI(&(sdp)->iqueue)
#else
#define sdGetWouldBlock(sdp) serial_lld_get_would_block((sdp))
#endif

/**
 * @brief   Direct write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly on the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @see     chnPutTimeout()
 *
 * @api
 */
#if !defined(serial_lld_put) || defined(__DOXYGEN__)
#define sdPut(sdp, b) chOQPut(&(sdp)->oqueue, b)
#else
#define sdPut(sdp, b) serial_lld_put((sdp), (b))
#endif

/**
 * @brief   Direct write to a @p SerialDriver with timeout specification.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly on the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @see     chnPutTimeout()
 *
 * @api
 */
#if !defined(serial_lld_put_timeout) || defined(__DOXYGEN__)
#define sdPutTimeout(sdp, b, t) chOQPutTimeout(&(sdp)->oqueue, b, t)
#else
#define sdPutTimeout(sdp, b, t) serial_lld_put_timeout((sdp), (b), (t))
#endif

/**
 * @brief   Direct read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @see     chnGetTimeout()
 *
 * @api
 */
#if !defined(serial_lld_get) || defined(__DOXYGEN__)
#define sdGet(sdp) chIQGet(&(sdp)->iqueue)
#else
#define sdGet(sdp) serial_lld_get((sdp))
#endif

/**
 * @brief   Direct read from a @p SerialDriver with timeout specification.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @see     chnGetTimeout()
 *
 * @api
 */
#if !defined(serial_lld_get_timeout) || defined(__DOXYGEN__)
#define sdGetTimeout(sdp, t) chIQGetTimeout(&(sdp)->iqueue, t)
#else
#define sdGetTimeout(sdp, t) serial_lld_get_timeout((sdp), (t))
#endif

/**
 * @brief   Direct blocking write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write from different channels implementations.
 *
 * @see     chnWrite()
 *
 * @api
 */
#if !defined(serial_lld_write) || defined(__DOXYGEN__)
#define sdWrite(sdp, b, n)                                                  \
  chOQWriteTimeout(&(sdp)->oqueue, b, n, TIME_INFINITE)
#else
#define sdWrite(sdp, b, n) serial_lld_write((sdp), (b), (n))
#endif

/**
 * @brief   Direct blocking write to a @p SerialDriver with timeout
 *          specification.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @see     chnWriteTimeout()
 *
 * @api
 */
#if !defined(serial_lld_write_timeout) || defined(__DOXYGEN__)
#define sdWriteTimeout(sdp, b, n, t)                                        \
  chOQWriteTimeout(&(sdp)->oqueue, b, n, t)
#else
#define sdWriteTimeout(sdp, b, n, t) serial_lld_write_timeout((sdp), (b), (n), (t))
#endif

/**
 * @brief   Direct non-blocking write to a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          writes directly to the output queue. This is faster but cannot
 *          be used to write to different channels implementations.
 *
 * @see     chnWriteTimeout()
 *
 * @api
 */
#if !defined(serial_lld_async_write) || defined(__DOXYGEN__)
#define sdAsynchronousWrite(sdp, b, n)                                      \
  chOQWriteTimeout(&(sdp)->oqueue, b, n, TIME_IMMEDIATE)
#else
#define sdAsynchronousWrite(sdp, b, n) serial_lld_async_write((sdp), (b), (n))
#endif

/**
 * @brief   Direct blocking read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @see     chnRead()
 *
 * @api
 */
#if !defined(serial_lld_read) || defined(__DOXYGEN__)
#define sdRead(sdp, b, n)                                                   \
  chIQReadTimeout(&(sdp)->iqueue, b, n, TIME_INFINITE)
#else
#define sdRead(sdp, b, n) serial_lld_read((sdp), (b), (n))
#endif

/**
 * @brief   Direct blocking read from a @p SerialDriver with timeout
 *          specification.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @see     chnReadTimeout()
 *
 * @api
 */
#if !defined(serial_lld_read_timeout) || defined(__DOXYGEN__)
#define sdReadTimeout(sdp, b, n, t)                                         \
  chIQReadTimeout(&(sdp)->iqueue, b, n, t)
#else
#define sdReadTimeout(sdp, b, n, t) serial_lld_read_timeout((sdp), (b), (n), (t))
#endif

/**
 * @brief   Direct non-blocking read from a @p SerialDriver.
 * @note    This function bypasses the indirect access to the channel and
 *          reads directly from the input queue. This is faster but cannot
 *          be used to read from different channels implementations.
 *
 * @see     chnReadTimeout()
 *
 * @api
 */
#if !defined(serial_lld_async_read) || defined(__DOXYGEN__)
#define sdAsynchronousRead(sdp, b, n)                                       \
  chIQReadTimeout(&(sdp)->iqueue, b, n, TIME_IMMEDIATE)
#else
#define sdAsynchronousRead(sdp, b, n) serial_lld_async_read((sdp), (b), (n))
#endif
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void sdInit(void);
  void sdObjectInit(SerialDriver *sdp, qnotify_t inotify, qnotify_t onotify);
  void sdStart(SerialDriver *sdp, const SerialConfig *config);
  void sdStop(SerialDriver *sdp);
  void sdIncomingDataI(SerialDriver *sdp, uint8_t b);
  msg_t sdRequestDataI(SerialDriver *sdp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_SERIAL */

#endif /* _SERIAL_H_ */

/** @} */
