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

static pthread_mutex_t simio_lock = PTHREAD_MUTEX_INITIALIZER;
static SOCKET fdout;

static struct {
  simio_cb_t  fp;
  void*       arg;
} input_cb;

/* define default ports for HAL drivers */
static struct sim_host_t {
  char*     ip_addr;
  uint16_t  port;
} sim_host[] = {
  {"127.0.0.1", 28000}, /* HAL_OUT */
  {NULL,        28001}  /* HAL_EXT */
};

/* accept threads for each HAL */
static pthread_t atp[sizeof(sim_host) / sizeof(struct sim_host_t)];

/* forward function declarations */
static char* id2hal(sim_hal_id_t);
static void accept_input(sim_hal_id_t);
static void* accept_thread(void*);
static void handle_input(int);
static SOCKET sock_init(void);
static void sock_listen(SOCKET, sim_hal_id_t );

/* stringy ids */
static char* id2hal(sim_hal_id_t hid) {
  switch (hid) {
    case HAL_OUT:
      return "HAL_OUT";
    case HAL_EXT:
      return "HAL_EXT";
    default:
      return "HAL_UNK";
  }
}

/* collect host and port data from the command line */
extern void sim_getopt(int argc, char **argv) {
  struct option longopts[] = {
    {"hal_output_host", required_argument, NULL, 'o'},
    {"hal_output_port", required_argument, NULL, 'O'},
    {"ext_listen_host", required_argument, NULL, 'e'},
    {"ext_listen_port", required_argument, NULL, 'E'},
    {NULL,              0,                 NULL,  0 }
  };

  int opt;
  while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {

      case 'o':
        sim_host[HAL_OUT].ip_addr = strdup(optarg);
        break;
      case 'O':
        sim_host[HAL_OUT].port = atoi(optarg);
        break;

      case 'e':
        sim_host[HAL_EXT].ip_addr = strdup(optarg);
        break;
      case 'E':
        sim_host[HAL_EXT].port = atoi(optarg);
        break;

      default:
        fprintf(stderr, "Usage: %s <options> \n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  /* reset getopt */
  optind = 1;
}

/* launch a thread to accept() connections for a driver */
static void accept_input(sim_hal_id_t hid) {
  int err;

  /* ignore repeated calls */
  if (atp[hid]) {
    fprintf(stderr, "ERROR simio already accepting connections\n");
    return;
  }

  if ((err = pthread_create(&atp[hid], NULL, accept_thread, (void*)hid))) {
    fprintf(stderr, "ERROR simio pthread_create() %s\n", strerror(err));
    exit(EXIT_FAILURE);
  }
}

static void* accept_thread(void *hid) {
  int fd;

  SOCKET sock = sock_init();
  sock_listen(sock, (sim_hal_id_t)hid);

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

/* connect to the output server */
extern void sim_connect_output(void) {
  struct sockaddr_in addr;

  /* ignore repeated calls */
  if (fdout) {
    fprintf(stderr, "ERROR simio already connected\n");
    return;
  }

  /* build addr struct */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(sim_host[HAL_OUT].port);

  if (!inet_aton(sim_host[HAL_OUT].ip_addr, &addr.sin_addr)) {
    fprintf(stderr, "ERROR simio invalid host %s\n", sim_host[HAL_OUT].ip_addr);
    exit(EXIT_FAILURE);
  }

  /* create socket and connect to remote */
  fdout = sock_init();
  if (connect(fdout, (struct sockaddr*)&addr, sizeof addr)) {
    fprintf(stderr, "ERROR simio connect %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

extern void sim_set_input_cb(sim_hal_id_t hid, simio_cb_t cb, void *arg) {
  input_cb.fp = cb;
  input_cb.arg = arg;
  accept_input(hid);
}

/**
 * @brief   Reads up to SIM_INPUT_MAX - 1 bytes from input and delivers the
 *          buffer to the registered callback.
 *
 * @param[in] p         socket file descriptor
 *
 * @notapi
 */
static void handle_input(int fd) {
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
    fprintf(stderr, "ERROR simio read %s\n", strerror(errno));
  } else {
    sim_io_lock();
    input_cb.fp(buf, input_cb.arg);
    sim_io_unlock();
  }

  close(fd);
}

extern void sim_printf(char *fmt, ...) {
  char buf[SIM_OUTPUT_MAX], *bufp = buf;
  int nb, left;
  va_list ap;

  /* fill buffer */
  va_start(ap, fmt);
  left = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  /* write to socket */
  while (left) {
    if ((nb = write(fdout, bufp, left)) <= 0) {
      fprintf(stderr, "ERROR simio write %s\n", strerror(errno));
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
      fprintf(stderr, "ERROR simio invalid listen host %s\n", sim_host[hid].ip_addr);
      exit(EXIT_FAILURE);
    }
  }

  if (bind(sock, (struct sockaddr*)&sad, sizeof(sad))) {
    fprintf(stderr, "ERROR simio bind %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (listen(sock, 1) != 0) {
    fprintf(stderr, "ERROR simio listen %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("simio %s driver listening on port %d\n", id2hal(hid), sim_host[hid].port);
}

extern void sim_io_lock(void) {
  pthread_mutex_lock(&simio_lock);
}

extern void sim_io_unlock(void) {
  pthread_mutex_unlock(&simio_lock);
}

extern void sim_io_stop() {
}
