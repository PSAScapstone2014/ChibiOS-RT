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
 * @brief   Helper macros and data structures.
 *
 * @addtogroup SIMUTIL
 * @{
 */

#ifndef SIMUTIL_H
#define SIMUTIL_H

#include <stdio.h>

/**
 * @brief console errors
 *
 * @notapi
 */
#define eprintf(fmt, args...) \
  (void)fprintf(stderr, "ERROR simio " fmt " at %s:%d\n" , ##args , __FILE__, __LINE__)

#define MSG_BLOCK_SIZE 256
#define DUMMY_HEADER "XXXXXX_XX -1234567890"

/**
 * @brief io message data
 */
typedef struct {
  char          *data;
  char          *dptr;
  size_t        dsz;
  size_t        dlen;
} sim_buf_t;

typedef enum {
  ST_HEADER,
  ST_DATA
} sim_state_t;

typedef struct {
  sim_state_t state;
  char        header[sizeof DUMMY_HEADER];
  char        *hptr;
  ssize_t     hlen;
  sim_buf_t   *buf;
} sim_msg_t;

extern sim_buf_t* sim_buf_alloc(size_t);
extern void sim_buf_free(sim_buf_t*);
extern void sim_buf_putc(sim_buf_t*, char);
extern void sim_buf_puts(sim_buf_t*, char*);
extern void sim_buf_setpos(sim_buf_t*, off_t pos);
extern size_t sim_buf_read(sim_buf_t *ibuf, void *obuf, size_t obufsz);
extern int sim_buf_eof(sim_buf_t*);

extern sim_msg_t* sim_msg_alloc(size_t);
extern void sim_msg_free(sim_msg_t*);

#endif /* SIMUTIL_H */

/** @} */
