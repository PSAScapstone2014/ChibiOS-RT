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

#define THD_SIZE 256

static pthread_t itp;
static pthread_t otp;
static int ofd;

static struct {
  simio_cb_t  fp;
  void*       arg;
} input_cb;

static SOCKET init_sock(uint16_t port) {
  struct sockaddr_in sad;
  struct protoent *prtp;
  SOCKET sock = INVALID_SOCKET, sockval = 1;
  socklen_t socklen = sizeof(sockval);


  if ((prtp = getprotobyname("tcp")) == NULL) {
    printf("ERROR in simio getprotobyname()\n");
    goto abort;
  }

  sock = socket(PF_INET, SOCK_STREAM, prtp->p_proto);
  if (sock == INVALID_SOCKET) {
    printf("ERROR in simio socket()\n");
    goto abort;
  }

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockval, socklen);

  memset(&sad, 0, sizeof(sad));
  sad.sin_family = AF_INET;
  sad.sin_addr.s_addr = INADDR_ANY;
  sad.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&sad, sizeof(sad))) {
    printf("ERROR in simio bind()\n");
    goto abort;
  }

  if (listen(sock, 1) != 0) {
    printf("ERROR simio listen()\n");
    goto abort;
  }

  printf("simio listening on port %d\n", port);
  return sock;

abort:
  if (sock != INVALID_SOCKET)
    close(sock);
  exit(EXIT_FAILURE);
}

/**
 * @brief   Reads a text line from input and sends it to the registered callback.
 *
 * @param[in] p         pointer to a @p BaseSequentialStream object
 * @return              Termination reason.
 * @retval RDY_OK       terminated by command.
 * @retval RDY_RESET    terminated by reset condition on the I/O channel.
 *
 * @notapi
 */
static void input_handler(int fd) {
  char buf[SIM_INPUT_MAX];
  uint32_t i;

  for (i = 0; i < sizeof(buf) - 1; i++) {
    uint8_t c;

    if (read((int)fd, &c, 1) <= 0)
      break;

    buf[i] = c;
  }

  /* always add null terminator */
  buf[i] = '\0';

  if (!errno)
    input_cb.fp(buf, input_cb.arg);

  close((int)fd);
}

void sim_printf(char *fmt, ...) {
  va_list ap;
  char buf[SIM_OUTPUT_MAX], *bufp = buf;
  int nb, left;
  sigset_t sigpipe_mask;

  va_start(ap, fmt);
  (void)vprintf(fmt, ap);
  va_end(ap);

  if (!ofd)
    return;

  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &sigpipe_mask, NULL) == -1) {
    printf("ERROR simio pthread_sigmask()\n");
    exit(1);
  }

  va_start(ap, fmt);
  left = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  while (left) {
    if ((nb = write(ofd, bufp, left)) <= 0) {
      fprintf(stderr, "ERROR simio write() fd %d %s\n", ofd, strerror(errno));
      ofd = 0;
      break;
    }
    left -= nb;
    bufp += nb;
  }
}

static void* accept_thread(void *port) {
  SOCKET sock = init_sock((SOCKET)port);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int fd;

  while (TRUE) {
    if ((fd = accept(sock, (struct sockaddr*)&addr, &addrlen)) == INVALID_SOCKET) {
      printf("ERROR simio accept() %s\n", strerror(errno));
      if (!(errno == ECONNABORTED || errno == EINTR))
          exit(EXIT_FAILURE);
    } else {
      if (pthread_self() == itp) {
        input_handler(fd);
      } else {
        ofd = fd;
      }
    }
  }

}

void sim_set_input_cb(simio_cb_t cb, void *arg) {
  input_cb.fp = cb;
  input_cb.arg = arg;
}

void sim_io_start(int portin, int portout) {
  int err;
  if (!itp && portin)
    if ((err = pthread_create(&itp, NULL, accept_thread, (void*)portin))) {
      printf("ERROR simio pthread_create() %s\n", strerror(err));
      exit(EXIT_FAILURE);
    }

  if (!otp && portout)
    if ((err = pthread_create(&otp, NULL, accept_thread, (void*)portout))) {
      printf("ERROR simio pthread_create() %s\n", strerror(err));
      exit(EXIT_FAILURE);
    }
}

void sim_io_stop() {
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
