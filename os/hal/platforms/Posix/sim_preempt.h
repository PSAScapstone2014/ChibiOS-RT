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
 * @file    sim_preempt.h
 * @brief   Helper macros and data structures.
 *
 * @addtogroup SIMPREEMPT
 * @{
 */

#ifndef SIM_PREEMPT_H
#define SIM_PREEMPT_H

#ifdef SIM_PREEMPT_REPLACE_FUNCTIONS
# define ACCEPT(a,b,c) sim_preempt_accept((a),(b),(c))
# define POLL(a,b,c) sim_preempt_poll((a),(b),(c))
# define READ(a,b,c) sim_preempt_read((a),(b),(c))
# define SELECT(a,b,c,d,e) sim_preempt_select((a),(b),(c),(d),(e))
#else
# define ACCEPT(a,b,c) accept((a),(b),(c))
# define POLL(a,b,c) poll((a),(b),(c))
# define READ(a,b,c) read((a),(b),(c))
# define SELECT(a,b,c,d,e) select((a),(b),(c),(d),(e))
#endif /* SIM_PREEMPT_REPLACE_FUNCTIONS */

#if defined(SIMULATOR) || defined(__DOXYGEN__)

#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "ch.h"

extern int sim_preempt_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

extern int sim_preempt_poll(struct pollfd *fds, nfds_t nfds, int timeout);

extern size_t sim_preempt_read_timeout(int fd, void *buf, size_t bufsz, systime_t timeout);
#define sim_preempt_read(a,b,c) sim_preempt_read_timeout((a),(b),(c),TIME_INFINITE)

extern int sim_preempt_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#endif /* SIMULATOR */
#endif /* SIM_PREEMPT_H */

/** @} */
