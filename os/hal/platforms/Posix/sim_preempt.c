#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ch.h"
#include "sim_preempt.h"

extern int errno;

#define SPIN_PROLOGUE(timeout, fd)                \
  struct pollfd fds =                             \
    { (fd), POLLIN|POLLERR|POLLHUP|POLLNVAL, 0 }; \
  int nfds;                                       \
  systime_t start = chTimeNow();                  \
  systime_t end   = start + (timeout);            \
                                                  \
  while ((timeout) == TIME_INFINITE ||            \
         chTimeIsWithin(start, end)) {            \
    nfds = poll(&fds, 1, 1);                      \
    if (fds.revents & ~POLLIN) {                  \
      close((fd));                                \
      return -1;                                  \
    }                                             \
    if (nfds > 0) {

#define SPIN_EPILOGUE(func)                       \
    } else if (nfds == 0) {                       \
      /* force reschedule */                      \
      chThdSleep(1);                              \
      continue;                                   \
    }                                             \
    else if (nfds < 0) {                          \
      fprintf(stderr, "ERROR sim_" func " %s\n", \
        strerror(errno));                         \
      if (errno == EINTR)                         \
        continue;                                 \
      else                                        \
        return -1;                                \
    }                                             \
  }

extern int sim_preempt_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  SPIN_PROLOGUE(TIME_INFINITE, sockfd)

  return accept(sockfd, addr, addrlen);

  SPIN_EPILOGUE("accept")

  /* timeout - shouldn't happen */
  errno = EWOULDBLOCK;
  return -1;
}

extern size_t sim_preempt_read_timeout(int fd, void *buf, size_t bufsz, systime_t timeout) {
  SPIN_PROLOGUE(timeout, fd)

  int nb = read(fd, buf, bufsz);
  if (nb <= 0)
    close(fd);
  return nb;

  SPIN_EPILOGUE("read")

  /* timeout */
  errno = EWOULDBLOCK;
  return -1;
}
