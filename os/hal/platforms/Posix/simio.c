#include "ch.h"
#include "hal.h"
#include "simio.h"
#include "chprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

pthread_t atp;
SOCKET fdout;

struct {
  simio_cb_t  fp;
  void*       arg;
} input_cb;

SOCKET sock_init(void);
void sock_listen(SOCKET sock, long port);
void* accept_thread(void *port);

void sim_accept_input(long port) {
  int err;

  /* ignore repeated calls */
  if (atp) {
    fprintf(stderr, "ERROR simio already accepting connections\n");
    return;
  }

  if ((err = pthread_create(&atp, NULL, accept_thread, (void*)port))) {
    fprintf(stderr, "ERROR simio pthread_create() %s\n", strerror(err));
    exit(EXIT_FAILURE);
  }
}

void sim_connect_output(char *host, uint16_t port) {
  struct sockaddr_in addr;

  /* build addr struct */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (!inet_aton(host, &addr.sin_addr)) {
    fprintf(stderr, "ERROR simio invalid host %s\n", host);
    exit(EXIT_FAILURE);
  }

  /* create socket and connect to remote */
  fdout = sock_init();
  if (connect(fdout, (struct sockaddr*)&addr, sizeof addr)) {
    fprintf(stderr, "ERROR simio connect %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void sim_set_input_cb(simio_cb_t cb, void *arg) {
  input_cb.fp = cb;
  input_cb.arg = arg;
}

void sim_io_stop() {
}

/**
 * @brief   Reads up to SIM_INPUT_MAX - 1 bytes from input and delivers the
 *          buffer to the registered callback.
 *
 * @param[in] p         socket file descriptor
 *
 * @notapi
 */
void handle_input(int fd) {
  char buf[SIM_INPUT_MAX];
  uint32_t i;

  /* fill buffer sans terminating null char */
  for (i = 0; i < sizeof(buf) - 1; i++) {
    uint8_t c;

    if (read(fd, &c, 1) <= 0)
      break;

    buf[i] = c;
  }

  /* always add null terminator */
  buf[i] = '\0';

  if (!errno)
    input_cb.fp(buf, input_cb.arg);
  else
    fprintf(stderr, "ERROR simio read %s\n", strerror(errno));

  close(fd);
}

void sim_printf(char *fmt, ...) {
  char buf[SIM_OUTPUT_MAX], *bufp = buf;
  int err, nb, left;
  sigset_t sigpipe_mask;
  va_list ap;

#ifndef DELETEME /* ToDo */
  va_start(ap, fmt);
  (void)vprintf(fmt, ap);
  va_end(ap);
#endif /* DELETEME */

  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  if ((err = pthread_sigmask(SIG_BLOCK, &sigpipe_mask, NULL))) {
    fprintf(stderr, "ERROR simio pthread_sigmask %s\n", strerror(err));
    exit(EXIT_FAILURE);
  }

  va_start(ap, fmt);
  left = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  while (left) {
    if ((nb = write(fdout, bufp, left)) <= 0) {
      fprintf(stderr, "ERROR simio write %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    left -= nb;
    bufp += nb;
  }
}

void* accept_thread(void *port) {
  SOCKET sock = sock_init();
  int fd;

  sock_listen(sock, (long)port);
  printf("simio listening on port %ld\n", (long)port);

  while (TRUE) {
    if ((fd = accept(sock, NULL, NULL)) == INVALID_SOCKET) {
      fprintf(stderr, "ERROR simio accept() %s\n", strerror(errno));
      if (!(errno == ECONNABORTED || errno == EINTR))
          exit(EXIT_FAILURE);
    } else {
      handle_input(fd);
    }
  }
}

// void sim_getopt(int argc, char **argv) {
//   int opt, sd1_port, sd2_port;

//   while ((opt = getopt(argc, argv, "i:o:")) != -1) {
//     switch (opt) {
//       case 'i':
//         sd1_port = atoi(optarg);
//         break;
//       case 'o':
//         sd2_port = atoi(optarg);
//         break;
//       default:
//         fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
//                         argv[0]);
//         exit(EXIT_FAILURE);
//     }
//   }
//   /* reset getopt */
//   optind = 1;
// }

SOCKET sock_init() {
  struct protoent *prtp;
  SOCKET sock = INVALID_SOCKET;

  if ((prtp = getprotobyname("tcp")) == NULL) {
    fprintf(stderr, "ERROR simio getprotobyname()\n");
    exit(EXIT_FAILURE);
  }

  sock = socket(PF_INET, SOCK_STREAM, prtp->p_proto);
  if (sock == INVALID_SOCKET) {
    fprintf(stderr, "ERROR simio socket()\n");
    exit(EXIT_FAILURE);
  }
  return sock;
}

void sock_listen(SOCKET sock, long port) {
  struct sockaddr_in sad;
  int sockval = 1;
  socklen_t socklen = sizeof(sockval);

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockval, socklen);

  memset(&sad, 0, sizeof(sad));
  sad.sin_family = AF_INET;
  sad.sin_addr.s_addr = INADDR_ANY;
  sad.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&sad, sizeof(sad))) {
    fprintf(stderr, "ERROR simio bind()\n");
    exit(EXIT_FAILURE);
  }

  if (listen(sock, 1) != 0) {
    fprintf(stderr, "ERROR simio listen()\n");
    exit(EXIT_FAILURE);
  }
}
