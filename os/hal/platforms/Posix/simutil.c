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
 * @file    simutil.c
 * @brief   Simulator helper functions.
 *
 * @addtogroup SIMUTIL
 * @{
 */

#include <stdlib.h>
#include <string.h>
#include "simutil.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

/**
 * @brief get a new message struct
 *
 * @notapi
 */
extern sim_buf_t* sim_buf_alloc(size_t len) {
  /* build the struct */
  sim_buf_t *buf = malloc(sizeof(sim_buf_t));
  if (!buf) {
    eprintf("out of memory");
    abort();
  }
  memset((void*)buf, '\0', sizeof(sim_buf_t));

  /* allocate room for message data */
  buf->data = malloc(len);
  if (!buf->data) {
    eprintf("out of memory");
    abort();
  }

  /* initialize members */
  buf->dptr = buf->data;
  buf->dsz = len;

  return buf;
}

static void sim_buf_realloc(sim_buf_t *buf, size_t len) {
  buf->data = realloc(buf->data, len);
  sim_buf_setpos(buf, buf->dlen);
  buf->dsz = len;
}

/**
 * @brief free all message data
 *
 * @notapi
 */
extern void sim_buf_free(sim_buf_t *buf) {
  free(buf->data);
  buf->dsz = 0;
  buf->dlen = 0;
  buf->data = NULL;
  buf->dptr = NULL;
  free(buf);
}

extern void sim_buf_putc(sim_buf_t *buf, char c) {
  if (buf->dlen == buf->dsz) {
    sim_buf_realloc(buf, buf->dsz + MSG_BLOCK_SIZE);
    if (!buf->data) {
      eprintf("out of memory");
      abort();
    }
  }
  *buf->dptr++ = c;
  buf->dlen++;
}

/**
 * @brief append null terminated string to the message
 *
 * @notapi
 */
extern void sim_buf_puts(sim_buf_t *buf, char *str) {
  while (*str)
    sim_buf_putc(buf, *str++);
}

extern void sim_buf_setpos(sim_buf_t *buf, off_t pos) {
    buf->dptr = buf->data + pos;
}

extern size_t sim_buf_read(sim_buf_t *ibuf, void *obuf, size_t obufsz) {
  size_t nb = MIN(ibuf->dlen, obufsz);
  memcpy(obuf, ibuf->dptr, nb);
  ibuf->dptr += nb;
  ibuf->dlen -= nb;
  return nb;
}

extern int sim_buf_eof(sim_buf_t *buf) {
    return !buf->dlen;
}

/**
 * @brief
 *
 * @notapi
 */
extern sim_msg_t* sim_msg_alloc(size_t len) {
  /* build the struct */
  sim_msg_t *msg = malloc(sizeof(sim_msg_t));
  if (!msg) {
    eprintf("out of memory");
    abort();
  }
  memset((void*)msg, '\0', sizeof(sim_msg_t));

  /* allocate room for message data */
  msg->buf = sim_buf_alloc(len);

  /* initialize members */
  msg->state = ST_HEADER;
  msg->hptr = msg->header;
  msg->hlen = 0;

  return msg;
}

/**
 * @brief
 *
 * @notapi
 */
extern void sim_msg_free(sim_msg_t *msg) {
  sim_buf_free(msg->buf);
  free(msg);
}

/** @} */
