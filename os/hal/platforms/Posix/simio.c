#include "ch.h"
#include "hal.h"
#include "simio.h"
#include "chprintf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

static pthread_mutex_t simio_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t listen_cond = PTHREAD_COND_INITIALIZER;

/* define default ports for HAL drivers */
static struct sim_host_t {
  char*     ip_addr;
  uint16_t  port;
} sim_host[] = {
  /* output ports */
  {"127.0.0.1", 27000}, /* PAL_OUT */
  {"127.0.0.1", 27011}, /* SD1_OUT */
  {"127.0.0.1", 27012}, /* SD2_OUT */

  /* input ports */
  {NULL,        28001}, /* EXT_INT */
  {NULL,        28011}, /* SD1_IN  */
  {NULL,        28012}, /* SD2_IN  */
};

/* accept threads and output fds for each HAL */
#define NUM_HANDLERS (sizeof(sim_host) / sizeof(struct sim_host_t))
static pthread_t atp[NUM_HANDLERS];
static SOCKET fdout[NUM_HANDLERS];
static SOCKET fdin[NUM_HANDLERS];

static struct {
  simio_cb_t  fp;
  void*       arg;
} input_cb[NUM_HANDLERS];

/* forward function declarations */
static char* hid2str(sim_hal_id_t);
static void* accept_thread(void*);
static void do_input_callback(sim_hal_id_t, int);
static SOCKET sock_init(void);
static void sock_listen(SOCKET, sim_hal_id_t );

/* stringy ids */
static char* hid2str(sim_hal_id_t hid) {
  switch (hid) {
    case PAL_OUT:
      return "PAL_OUT";
    case SD1_OUT:
      return "SD1_OUT";
    case SD1_IN:
      return "SD1_IN";
    case SD2_OUT:
      return "SD2_OUT";
    case SD2_IN:
      return "SD2_IN";
    case EXT_INT:
      return "EXT_INT";
    default:
      return "HAL_UNK";
  }
}

/* collect host and port data from the command line */
extern void sim_getopt(int argc, char **argv) {
  struct option longopts[] = {
    {"pal_output_host", required_argument, NULL, 'a'},
    {"pal_output_port", required_argument, NULL, 'A'},

    {"sd1_output_host", required_argument, NULL, 'b'},
    {"sd1_output_port", required_argument, NULL, 'B'},
    {"sd1_listen_host", required_argument, NULL, 'c'},
    {"sd1_listen_port", required_argument, NULL, 'C'},

    {"sd2_output_host", required_argument, NULL, 'd'},
    {"sd2_output_port", required_argument, NULL, 'D'},
    {"sd2_listen_host", required_argument, NULL, 'e'},
    {"sd2_listen_port", required_argument, NULL, 'E'},

    {"ext_listen_host", required_argument, NULL, 'f'},
    {"ext_listen_port", required_argument, NULL, 'F'},

    {NULL,              0,                 NULL,  0 }
  };

  int opt;
  while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {

      case 'a': sim_host[PAL_OUT].ip_addr = strdup(optarg); break;
      case 'A': sim_host[PAL_OUT].port = atoi(optarg); break;

      case 'b': sim_host[SD1_OUT].ip_addr = strdup(optarg); break;
      case 'B': sim_host[SD1_OUT].port = atoi(optarg); break;
      case 'c': sim_host[SD1_IN].ip_addr = strdup(optarg); break;
      case 'C': sim_host[SD1_IN].port = atoi(optarg); break;

      case 'd': sim_host[SD2_OUT].ip_addr = strdup(optarg); break;
      case 'D': sim_host[SD2_OUT].port = atoi(optarg); break;
      case 'e': sim_host[SD2_IN].ip_addr = strdup(optarg); break;
      case 'E': sim_host[SD2_IN].port = atoi(optarg); break;

      case 'f': sim_host[EXT_INT].ip_addr = strdup(optarg); break;
      case 'F': sim_host[EXT_INT].port = atoi(optarg); break;

      default:
        fprintf(stderr, "Usage: %s <options> \n", argv[0]); /* ToDo: option list help */
        exit(EXIT_FAILURE);
    }
  }
  /* reset getopt */
  optind = 1;
}

/* pthread_cond_wait wrapper */
static void wait_thread_listen(pthread_mutex_t *lock) {
  int err = pthread_cond_wait(&listen_cond, lock);
  if (err) {
    fprintf(stderr, "ERROR simio wait_thread_listen %s\n",
      strerror(err));
    exit(EXIT_FAILURE);
  }
}

/* pthread_cond_signal wrapper */
static void signal_thread_listen(void) {
  int err = pthread_cond_signal(&listen_cond);
  if (err) {
    fprintf(stderr, "ERROR simio signal_thread_listen %s\n",
      strerror(err));
    exit(EXIT_FAILURE);
  }
}

/* launch a thread to accept() connections for a driver */
extern void sim_accept_input(sim_hal_id_t hid) {
  static pthread_mutex_t wait_lock = PTHREAD_MUTEX_INITIALIZER;
  int err;

  /* lock waiting for the thread's listen() */
  pthread_mutex_lock(&wait_lock);

  /* ignore repeated calls */
  if (atp[hid]) {
    fprintf(stderr, "ERROR simio already accepting connections\n");
    return;
  }

  if ((err = pthread_create(&atp[hid], NULL, accept_thread, (void*)hid))) {
    fprintf(stderr, "ERROR simio pthread_create() %s\n", strerror(err));
    exit(EXIT_FAILURE);
  }

  /* wait for the thread to listen() before returning */
  wait_thread_listen(&wait_lock);
}

static void* accept_thread(void *arg) {
  sim_hal_id_t hid = (sim_hal_id_t)arg;
  int fd;

  SOCKET sock = sock_init();
  sock_listen(sock, hid);

  /* signal the main thread to resume   *
   * now that we can accept connections */
  signal_thread_listen();

  while (TRUE) {
    if ((fd = accept(sock, NULL, NULL)) == INVALID_SOCKET) {
      fprintf(stderr, "ERROR simio accept() %s\n", strerror(errno));
      if (!(errno == ECONNABORTED || errno == EINTR))
          exit(EXIT_FAILURE);
    } else {
      if (input_cb[hid].fp) {
        /* "event" style handler */
        do_input_callback(hid, fd);
      } else {
        if (!fdin[hid]) {
          /* direct read handler */
          fdin[hid] = fd;
        } else {
          /* ignore multiple connects by closing any new ones */
          if (close(fd))
            fprintf(stderr, "ERROR simio accept_thread close %s\n",
                    strerror(errno));
        }
      }
    }
  }
}

/* connect to the output server */
extern void sim_connect_output(sim_hal_id_t hid) {
  struct sockaddr_in addr;

  /* ignore repeated calls */
  if (fdout[hid]) {
    fprintf(stderr, "ERROR simio %s already connected\n",
      hid2str(hid));
    return;
  }

  /* build addr struct */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(sim_host[hid].port);

  if (!inet_aton(sim_host[hid].ip_addr, &addr.sin_addr)) {
    fprintf(stderr, "ERROR simio %s invalid host %s\n",
      hid2str(hid), sim_host[hid].ip_addr);
    exit(EXIT_FAILURE);
  }

  /* create socket and connect to remote */
  fdout[hid] = sock_init();
  if (connect(fdout[hid], (struct sockaddr*)&addr, sizeof addr)) {
    fprintf(stderr, "ERROR simio %s connect %s:%d %s\n",
      hid2str(hid), sim_host[hid].ip_addr, sim_host[hid].port, strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("simio %s driver connected to %s:%d\n",
    hid2str(hid), sim_host[hid].ip_addr, sim_host[hid].port);
}

extern int sim_read_timeout(sim_hal_id_t hid, void *buf, size_t bufsz, int timeout) {
  struct pollfd fd = { fdin[hid], POLLIN, 0 };
  int ready = poll(&fd, 1, timeout);
  if (ready > 0)
    return sim_read(hid, buf, bufsz);
  else
    return ready;
}

extern ssize_t sim_read(sim_hal_id_t hid, void *buf, size_t bufsz) {
  ssize_t nb = read(fdin[hid], buf, bufsz);

  /* accept other connections on error or EOF */
  if (nb <= 0)
    fdin[hid] = 0;

  return nb;
}

extern void sim_set_input_cb(sim_hal_id_t hid, simio_cb_t cb, void *arg) {
  input_cb[hid].fp = cb;
  input_cb[hid].arg = arg;
  sim_accept_input(hid);
}

/**
 * @brief   Reads up to SIM_INPUT_MAX - 1 bytes from input and delivers the
 *          buffer to the registered callback.
 *
 * @param[in] p         socket file descriptor
 *
 * @notapi
 */
static void do_input_callback(sim_hal_id_t hid, int fd) {
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

  if (errno) {
    fprintf(stderr, "ERROR simio read %s\n",
      strerror(errno));
  } else {
    /* lock the idle thread to prevent chibios lock issues */
    sim_io_lock();
    input_cb[hid].fp(buf, input_cb[hid].arg);
    sim_io_unlock();
  }

  close(fd);
}

extern ssize_t sim_write(sim_hal_id_t hid, void *buf, size_t bufsz) {
  if (!fdout[hid]) {
    fprintf(stderr, "ERROR sim_write %s not connected\n",
      hid2str(hid));
    exit(EXIT_FAILURE);
  }

  return write(fdout[hid], buf, bufsz);
}

extern void sim_printf(sim_hal_id_t hid, char *fmt, ...) {
  char buf[SIM_OUTPUT_MAX], *bufp = buf;
  int nb, left;
  va_list ap;

  if (!fdout[hid]) {
    fprintf(stderr, "ERROR sim_printf %s not connected\n",
      hid2str(hid));
    exit(EXIT_FAILURE);
  }

  /* fill buffer */
  va_start(ap, fmt);
  left = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  /* write to socket */
  while (left) {
    if ((nb = write(fdout[hid], bufp, left)) <= 0) {
      fprintf(stderr, "ERROR simio write %s\n",
        strerror(errno));
      exit(EXIT_FAILURE);
    }
    left -= nb;
    bufp += nb;
  }
}

static SOCKET sock_init() {
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

static void sock_listen(SOCKET sock, sim_hal_id_t hid) {
  struct sockaddr_in sad;
  int sockval = 1;
  socklen_t socklen = sizeof(sockval);

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockval, socklen);

  memset(&sad, 0, sizeof(sad));
  sad.sin_family = AF_INET;
  sad.sin_port = htons(sim_host[hid].port);

  sad.sin_addr.s_addr = INADDR_ANY;
  if (sim_host[hid].ip_addr) {
    if (!inet_aton(sim_host[hid].ip_addr, &sad.sin_addr)) {
      fprintf(stderr, "ERROR simio %s invalid listen host %s\n",
        hid2str(hid), sim_host[hid].ip_addr);
      exit(EXIT_FAILURE);
    }
  }

  if (bind(sock, (struct sockaddr*)&sad, sizeof(sad))) {
    fprintf(stderr, "ERROR simio %s bind %s\n",
      hid2str(hid), strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (listen(sock, 1) != 0) {
    fprintf(stderr, "ERROR simio %s listen %s\n",
      hid2str(hid), strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("simio %s driver listening on port %d\n",
    hid2str(hid), sim_host[hid].port);
}

extern void sim_io_lock(void) {
  pthread_mutex_lock(&simio_lock);
}

extern void sim_io_unlock(void) {
  pthread_mutex_unlock(&simio_lock);
}

extern void sim_io_stop() {
}
