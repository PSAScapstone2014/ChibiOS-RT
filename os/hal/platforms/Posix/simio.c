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
 * @file    simio.c
 * @brief   Simulator network IO code.
 *
 * @addtogroup SIMIO
 * @{
 */

#if defined(SIMULATOR) || defined(__DOXYGEN__)

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ch.h"
#include "hal.h"
#include "simio.h"
#include "simutil.h"
#include "sim_preempt.h"

/**
 * @brief   globals to handle a multiplexed socket
 */
static msg_t readq[HID_COUNT][MB_QUEUE_SIZE];
static Mailbox read_mb[HID_COUNT];
// static Mailbox write_mb[HID_COUNT]; /* TODO */
static WORKING_AREA(wap, 512);

/**
 * @brief   Define default ports for HAL drivers
 */
static struct sim_host_t {
  char*     ip_addr;
  uint16_t  port;
  SOCKET    sock;
} sim_host = { "127.0.0.1", 27000, 0 };

/**
 * @brief   Forward function declarations
 */
static char* hid2str(sim_hal_id_t*);
static SOCKET _sim_socket(void);
static int _sim_connect(void);
static msg_t read_thread(void *arg);

/**
 * @brief   One time initialization of data structures
 *
 * @notapi
 */
static void _sim_init_once(void) {
  static bool once = FALSE;
  int i;
  if (!once) {
    once = TRUE;
    for (i = 0; i < HID_COUNT; i++) {
      chMBInit(&read_mb[i], readq[i], HID_COUNT);
    }
  }
}

/**
 * @brief   Collect host and port data from the command line
 *
 * @param[in] argc      command line argument count
 * @param[in] argc      command line argument vector

 * @api
 */
extern void sim_getopt(int argc, char **argv) {
  struct option longopts[] = {
    {"sim_host", required_argument, NULL, 'h'},
    {"sim_port", required_argument, NULL, 'p'},
    {      NULL,                 0, NULL,  0 }
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "h:p:", longopts, NULL)) != -1) {
    switch (opt) {

      case 'h': sim_host.ip_addr = strdup(optarg); break;
      case 'p': sim_host.port = atoi(optarg); break;

      default:
        eprintf("usage: %s <options>", argv[0]); /* ToDo: option list help */
        exit(EXIT_FAILURE);
    }
  }
  /* reset getopt */
  optind = 1;
}

/**
 * @brief get string name for hid
 *
 * @notapi
 */
static char* hid2str(sim_hal_id_t *hid) {
  switch (hid->id) {
    case SIM_IO: return "SIM_IO";
    case PAL_IO: return "PAL_IO";
    case SD1_IO: return "SD1_IO";
    case SD2_IO: return "SD2_IO";
    case EXT_IO: return "EXT_IO";
    case SPI_IO: return "SPI_IO";
    case SDC_IO: return "SDC_IO";
    case I2C_IO: return "I2C_IO";
    default:     return "UNK_IO";
  }
}

/**
 * @brief get enum for hid string
 *
 * @notapi
 */
static hid_t str2hid(char *hid) {
  if (!strcmp(hid, "PAL_IO")) return PAL_IO;
  else if (!strcmp(hid, "SD1_IO")) return SD1_IO;
  else if (!strcmp(hid, "SD2_IO")) return SD2_IO;
  else if (!strcmp(hid, "EXT_IO")) return EXT_IO;
  else if (!strcmp(hid, "SPI_IO")) return SPI_IO;
  else if (!strcmp(hid, "SDC_IO")) return SDC_IO;
  else if (!strcmp(hid, "I2C_IO")) return I2C_IO;
  eprintf("no such queue %s", hid);
  return SIM_IO;
}

/**
 * @brief   Format data using the simulator protocol.
 * @details Each message is formatted with a header followed
 *          by a hexadecimal encoded string and newline.
 * @note    This function allocates memory.
 *
 * @param[in] hid       hal id to use in the header
 * @param[in] buf       the message data
 * @param[in] bufsz     the size of buf
 *
 * @return              a newly allocated buffer
 *
 * @notapi
 */
static sim_buf_t* _sim_encode(sim_hal_id_t *hid, void *buf, size_t bufsz) {
  sim_buf_t *code = sim_buf_alloc(sizeof(DUMMY_HEADER "\t") + bufsz*2);
  char hex[3];
  size_t i;

  sim_buf_puts(code, hid2str(hid));
  sim_buf_puts(code, "\t");

  for (i = 0; i < bufsz; i++) {
    snprintf(hex, sizeof hex, "%02x", ((uint8_t*)buf)[i]);
    sim_buf_puts(code, hex);
  }

  sim_buf_puts(code, "\n");

  return code;
}

/**
 * @brief   Decode hexadecimal encodings in place.
 *
 * @param[in,out] buf   the encoded data
 *
 * @notapi
 */
static void _sim_decode(sim_buf_t *buf) {
  char byte[3];
  size_t i;

  for (i = 0; i < buf->dlen/2; i++) {
    snprintf(byte, sizeof byte, "%c%c", buf->data[i*2], buf->data[i*2+1]);
    buf->data[i] = (uint8_t)strtoul(byte, NULL, 16);
  }

  /* done writing - reset for reads */
  sim_buf_setpos(buf, 0);
  buf->dlen = i;
}

/**
 * @brief   Post a message from a VHA to the reader queue
 *          for consumption by a LLD.
 *
 * @param[in] msg       The message to post.
 *
 * @notapi
 */
static void _sim_enqueue(sim_msg_t *msg) {
  hid_t hid = str2hid(msg->header);
  msg_t status;

  while (TRUE) {
    status = chMBPost(&read_mb[hid], (msg_t)msg, S2ST(1));
    if (status == RDY_OK)
      break;
    if (status == RDY_TIMEOUT)
      eprintf("read queue full");
  };
}

/**
 * @brief   Parse a buffer returned from read() into a message
 *          structure.
 * @details Read() may only return a partial buffer. This function
 *          holds the message pointer until such time that the
 *          read has been completed and then queues it into the
 *          LLD reader queue.
 *
 * @param[in] buf       the raw data
 * @param[in,out] mptr  pointer to a pointer to a message
 *
 * @notapi
 */
static void parse_buf(sim_buf_t *buf, sim_msg_t **mptr) {
  sim_msg_t *msg = *mptr;
  size_t i;

  for (i = 0; i < buf->dlen; i++) {
    switch (msg->state) {

      case ST_HEADER:
        if (buf->data[i] == '\t') {
          msg->state = ST_DATA;
          break;
        }
        *msg->hptr++ = buf->data[i];
        msg->hlen++;
        break;

      case ST_DATA:
        if (buf->data[i] == '\n') {
          _sim_decode(msg->buf);
          _sim_enqueue(msg);
          msg = *mptr = sim_msg_alloc(MSG_BLOCK_SIZE);
          break;
        }
        sim_buf_putc(msg->buf, buf->data[i]);
        break;

      default:
        eprintf("unknown state");
        abort();
    }
  }
}

/**
 * @brief   The thread responsible for reading from a VHA.
 *
 * @notapi
 */
static msg_t read_thread(void *arg) {
  sim_msg_t *msg = sim_msg_alloc(MSG_BLOCK_SIZE);
  sim_buf_t *buf = sim_buf_alloc(MSG_BLOCK_SIZE);
  ssize_t nb;

  (void)arg;

  while (TRUE) {
    if (!sim_host.sock) {
      chThdSleep(S2ST(1));
      if (_sim_connect() < 0)
        continue;
    }

    nb = sim_preempt_read(sim_host.sock, buf->data, buf->dsz);
    buf->dlen = nb;

    if (nb < 0) {
      eprintf("read %s", strerror(errno));
      (void)sim_disconnect();
    }

    else if (nb == 0) {
      sim_msg_free(msg);
      msg = sim_msg_alloc(MSG_BLOCK_SIZE);
    }

    else {
      parse_buf(buf, &msg);
    }
  }

  /* squash warning */
  return 0;
}

/**
 * @brief   Fetch the next message from the queue and drain its
 *          buffer to emulate an actual read() call
 *
 * @param[in] hid       hal id
 * @param[out] buf      buffer to be filled
 * @param[in] bufsz     the size of buf
 * @param[in] timeout   maximum time to wait before returning
 *
 * @return              the number of bytes read or, if negative an error
 *
 * @api
 */
extern ssize_t sim_read_timeout(sim_hal_id_t *hid, void *buf, size_t bufsz, int timeout) {
  static sim_msg_t *msg = NULL;
  msg_t status = RDY_OK;
  ssize_t nb;

  if (!sim_host.sock) {
    if (_sim_connect() < 0) {
      errno = EBADF;
      return -1;
    }
  }

  if (!msg) {
    status = chMBFetch(&read_mb[hid->id], (msg_t*)&msg, timeout);
  }

  if (status == RDY_OK) {
    nb = sim_buf_read(msg->buf, buf, bufsz);
    if (sim_buf_eof(msg->buf)) {
      sim_msg_free(msg);
      msg = NULL;
    }
  }

  return status == RDY_OK ? nb : status;
}

/**
 * @brief   encode and write buffer to multiplexed IO stream
 *
 * @param[in] hid       hal identifier
 * @param[in] buf       data to be written to the VHA
 * @param[in] bufsz     size of buf
 *
 * @return              the number of byte written or negative if an
 *                      error occurred
 * @api
 */
extern int sim_write(sim_hal_id_t *hid, void *buf, size_t bufsz) {
  sim_buf_t *code;
  int nb = 0;

  if (!sim_host.sock) {
    if (_sim_connect() < 0) {
      errno = EBADF;
      return -1;
    }
  }

  code = _sim_encode(hid, buf, bufsz);

  /* this section of code could probably use a *
   * write thread with queued messages - the   *
   * write isn't guaranteed to write the       *
   * entire buffer and could be preempted      */
  while (code->dlen) {
    if ((nb = write(sim_host.sock, code->data, code->dlen)) <= 0)
      break;
    code->dlen -= nb;
    code->dptr += nb;
  }

  if (nb < 0) {
    eprintf("%s write %s", hid2str(hid), strerror(errno));
    (void)sim_disconnect();
  }

  // end lock

  sim_buf_free(code);

  return nb < 0 ? nb : (int)bufsz;
}

/**
 * @brief   write formatted data to io stream
 * @note    the output may not exceed SIM_PRINTF_MAX bytes
 *
 * @param[in] hid       hal identifier
 * @param[in] fmt       printf-style format string
 * @param[in] ...       variable arguments
 *
 * @return              the number of byte written or negative if an
 *                      error occurred
 * @api
 */
extern int sim_printf(sim_hal_id_t *hid, char *fmt, ...) {
  char buf[SIM_PRINTF_MAX], *bufp = buf;
  va_list ap;
  int nb;

  /* fill buffer */
  va_start(ap, fmt);
  nb = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  /* write to socket */
  return sim_write(hid, bufp, nb);
}

/**
 * @brief   Disconnect IO stream
 * @note    Will reconnect if another IO call is used
 *
 * @return              0 on success, -1 on failure
 *
 * @api
 */
extern int sim_disconnect() {
  int rv = close(sim_host.sock);
  sim_host.sock = 0;
  return rv;
}

/**
 * @brief   Establish network IO
 * @note    Spawns VHA reader thread. Implicitly
 *          called from other IO functions.
 *
 * @return              0 on success, -1 on failure
 *
 * @notapi
 */
static int _sim_connect() {
  struct sockaddr_in addr;

  _sim_init_once();

  /* ignore repeated calls */
  if (sim_host.sock) {
    eprintf("already connected");
    return 0;
  }

  (void)chThdCreateStatic(wap, sizeof(wap), NORMALPRIO, read_thread, NULL);

  /* build addr struct */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(sim_host.port);

  if (!inet_aton(sim_host.ip_addr, &addr.sin_addr)) {
    eprintf("invalid host %s", sim_host.ip_addr);
    exit(EXIT_FAILURE);
  }

  /* create socket and connect to remote */
  sim_host.sock = _sim_socket();
  if (connect(sim_host.sock, (struct sockaddr*)&addr, sizeof addr)) {
    eprintf("connect %s:%d %s",
      sim_host.ip_addr, sim_host.port, strerror(errno));
    sim_host.sock = 0;
    return -1;
  }

  printf("simio connected to %s:%d\n",
    sim_host.ip_addr, sim_host.port);

  return 0;
}

/**
 * @brief   Initialize a new TCP socket
 *
 * @return              A socket number
 *
 * @notapi
 */
static SOCKET _sim_socket() {
  struct protoent *prtp;
  SOCKET sock = INVALID_SOCKET;

  if ((prtp = getprotobyname("tcp")) == NULL) {
    eprintf("getprotobyname");
    exit(EXIT_FAILURE);
  }

  sock = socket(PF_INET, SOCK_STREAM, prtp->p_proto);
  if (sock == INVALID_SOCKET) {
    eprintf("socket");
    exit(EXIT_FAILURE);
  }
  return sock;
}

#endif /* SIMULATOR */

/** @} */
