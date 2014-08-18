#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ch.h"
#include "sim_preempt.h"

static int spin_poll(systime_t timeout, int fd) {
  struct pollfd fds =
    { fd, POLLIN|POLLERR|POLLHUP|POLLNVAL, 0 };

  systime_t start = chTimeNow();
  systime_t end   = start + timeout;

  while (timeout == TIME_INFINITE ||
         chTimeIsWithin(start, end)) {

    int nfds = poll(&fds, 1, 1);
    if (fds.revents & ~POLLIN) {
      return -1;
    }

    if (nfds < 0) {
      if (errno == EINTR) {
        continue;
      }
      else {
        fprintf(stderr, "ERROR spin_poll %s\n",
          strerror(errno));
        return -1;
      }
    }
    else if (nfds == 0) {
      /* force reschedule */
      chThdSleep(1);
      continue;

    } else {
      return nfds;
    }
  }

  /* timeout */
  return -1;
}

extern int sim_preempt_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  if (spin_poll(TIME_INFINITE, sockfd) > 0)
    return accept(sockfd, addr, addrlen);

  /* timeout - shouldn't happen */
  errno = EWOULDBLOCK;
  return -1;
}

extern size_t sim_preempt_read_timeout(int fd, void *buf, size_t bufsz, systime_t timeout) {
  if (spin_poll(timeout, fd) > 0) {
    int nb = read(fd, buf, bufsz);
    if (nb <= 0)
      close(fd);
    return nb;
  }

  /* timeout */
  errno = EWOULDBLOCK;
  return -1;
}
