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
 * @file    sim_preempt.c
 * @brief   Preemptable socket functions.
 *
 * @addtogroup SIMPREEMPT
 * @{
 */

#if defined(SIMULATOR) || defined(__DOXYGEN__)

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ch.h"
#include "sim_preempt.h"

/* Tunable setting adjusts responsiveness of *
 * the scheduler when spinning on a socket.  *
 * Setting this value too low will cause the *
 * scheduler to lag. Too high and the socket *
 * response will lag.                        */
#define SPIN_SLEEP_TICKS 3

/**
 * @brief   Spin around a fd until data becomes
 *          available or there is an error
 * @note    Allows ChibiOS to preempt linux system
 *          calls that would normally block
 *
 * @param[in] timeout   Ticks before returning
 * @param[in] fd        The fd to check
 *
 * @return              1 if data is ready
 *                      0 if timeout
 *                      -1 if error
 *
 */
static int spin_poll(systime_t timeout, int fd) {
  struct pollfd fds =
    { fd, POLLIN, 0 };

  systime_t start = chTimeNow();
  systime_t end   = start + timeout;

  while (timeout == TIME_INFINITE ||
         chTimeIsWithin(start, end)) {

    int nfds = poll(&fds, 1, 0);

    /* fd error if not POLLIN */
    if (fds.revents & ~POLLIN) {
      return -1;
    }

    /* poll error */
    if (nfds < 0) {

      /* try again if interrupted */
      if (errno == EINTR) {
        continue;
      }

      /* otherwise print the error and return */
      else {
        fprintf(stderr, "ERROR spin_poll %s\n",
          strerror(errno));
        return -1;
      }
    }

    /* No data available    *
     * force the reschedule */
    else if (nfds == 0) {
      chThdSleep(SPIN_SLEEP_TICKS);
      continue;
    }

    /* Success */
    else {
      return nfds;
    }
  }

  /* timeout */
  return 0;
}

/**
 * @brief   preemptable accept()
 *
 * @param[in] sockfd    The socket to check
 * @param[out] addr     The client address
 * @param[out] addrlen  The length of addr
 *
 * @return              The fd on success
 *                      -1 on error
 */
extern int sim_preempt_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  if (spin_poll(TIME_INFINITE, sockfd) > 0)
    return accept(sockfd, addr, addrlen);

  /* spin_poll error */
  return -1;
}

/**
 * @brief   Preemptable read() with timeout
 * @note    Sets errno to EWOULDBLOCK on timeout
 *
 * @param[in] fd        The fd to read from
 * @param[out] buf      The buffer to fill
 * @param[in] bufsz     The size of buf
 * @param[in] timeout   Timeout in system ticks
 *
 * @return              The number of bytes read on success
 *                      0 on EOF
 *                      -1 on error or timeout
 */
extern size_t sim_preempt_read_timeout(int fd, void *buf, size_t bufsz, systime_t timeout) {
  int rv = 0;
  if ((rv = spin_poll(timeout, fd)) > 0) {
    int nb = read(fd, buf, bufsz);
    if (nb <= 0)
      close(fd);
    return nb;
  }

  /* timeout */
  else if (rv == 0) {
    errno = EWOULDBLOCK;
  }

  return -1;
}

/**
 * @brief   Preemptable poll()
 * @note    errno is set on error
 *
 * @param[in] fds       The fds to check
 * @param[in] nfds      The number of fds
 * @param[in] timeout   Timeout in system ticks
 *
 * @return              The number of ready fds
 *                      0 on timeout
 *                      -1 on error
 */
extern int sim_preempt_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  int rv = 0;

  /* set loop spin timeout */
  systime_t start = chTimeNow();
  systime_t end   = start + MS2ST(timeout);

  /* spin poll until error, spin timeout, or fds are waiting */
  while (chTimeIsWithin(start, end)) {
    if ((rv = poll(fds, nfds, 0)) != 0)
      return rv;
    /* force reschedule */
    chThdSleep(SPIN_SLEEP_TICKS);
  }

  /* timeout */
  return rv;
}

/**
 * @brief   Preemptable select()
 * @note    errno is set on error
 *
 * @param[in] nfds            The number of fds to check
 * @param[in,out] readfds     FDs to watch for reads
 * @param[in,out] writefds    FDs to watch for writes
 * @param[in,out] exceptfds   FDs to watch for exceptions
 * @param[in] timeout         Timeout in system ticks
 *
 * @return              The number of ready fds
 *                      0 on timeout
 *                      -1 on error
 */
extern int sim_preempt_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
  struct timeval now, end, no_wait = {0,0};
  int rv = 0;

  /* set loop spin timeout */
  if ((rv = gettimeofday(&now, NULL)))
    return rv;
  timeradd(&now, timeout, &end);

  /* spin select until error, spin timeout, or fds are waiting */
  while (timercmp(&now, &end, <)) {
    if ((rv = select(nfds, readfds, writefds, exceptfds, &no_wait)) != 0)
      return rv;
    /* update timer */
    if ((rv = gettimeofday(&now, NULL)))
      return rv;
    /* force reschedule */
    chThdSleep(SPIN_SLEEP_TICKS);
  }

  /* timeout */
  return rv;
}

#endif /* SIMULATOR */

/** @} */
