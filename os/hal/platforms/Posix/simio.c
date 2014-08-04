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
#include "sim_preempt.h"

/* console errors */
#define eprintf(fmt, args...) \
  (void)fprintf(stderr, "ERROR simio " fmt " at %s:%d\n" , ##args , __FILE__, __LINE__)

/* define default ports for HAL drivers */
static struct sim_host_t {
  char*     ip_addr;
  uint16_t  port;
} sim_host[] = {
  /* io ports */
  {"127.0.0.1", 27000}, /* PAL_IO */
  {"127.0.0.1", 27010}, /* SD1_IO */
  {"127.0.0.1", 27011}, /* SD2_IO */
  {"127.0.0.1", 27020}, /* EXT_IO */
  {"127.0.0.1", 27030}, /* SPI_IO */
};

/* accept threads and output fds for each HAL */
#define NUM_HANDLERS ( sizeof(sim_host) / sizeof(struct sim_host_t) )
static SOCKET simfd[NUM_HANDLERS];

/* forward function declarations */
static char* hid2str(sim_hal_id_t);
static SOCKET sock_init(void);

/* collect host and port data from the command line */
extern void sim_getopt(int argc, char **argv) {
  struct option longopts[] = {
    {"pal_host", required_argument, NULL, 'a'},
    {"pal_port", required_argument, NULL, 'A'},

    {"sd1_host", required_argument, NULL, 'b'},
    {"sd1_port", required_argument, NULL, 'B'},

    {"sd2_host", required_argument, NULL, 'c'},
    {"sd2_port", required_argument, NULL, 'C'},

    {"ext_host", required_argument, NULL, 'd'},
    {"ext_port", required_argument, NULL, 'D'},

    {"spi_host", required_argument, NULL, 'e'},
    {"spi_port", required_argument, NULL, 'E'},

    {NULL,              0,                 NULL,  0 }
  };

  int opt;
  while ((opt = getopt_long_only(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {

      case 'a': sim_host[PAL_IO].ip_addr = strdup(optarg); break;
      case 'A': sim_host[PAL_IO].port = atoi(optarg); break;

      case 'b': sim_host[SD1_IO].ip_addr = strdup(optarg); break;
      case 'B': sim_host[SD1_IO].port = atoi(optarg); break;

      case 'c': sim_host[SD2_IO].ip_addr = strdup(optarg); break;
      case 'C': sim_host[SD2_IO].port = atoi(optarg); break;

      case 'd': sim_host[EXT_IO].ip_addr = strdup(optarg); break;
      case 'D': sim_host[EXT_IO].port = atoi(optarg); break;

      case 'e': sim_host[EXT_IO].ip_addr = strdup(optarg); break;
      case 'E': sim_host[EXT_IO].port = atoi(optarg); break;

      default:
        eprintf("usage: %s <options>", argv[0]); /* ToDo: option list help */
        exit(EXIT_FAILURE);
    }
  }
  /* reset getopt */
  optind = 1;
}

/* stringy ids */
static char* hid2str(sim_hal_id_t hid) {
  switch (hid) {
    case PAL_IO:
      return "PAL_IO";
    case SD1_IO:
      return "SD1_IO";
    case SD2_IO:
      return "SD2_IO";
    case EXT_IO:
      return "EXT_IO";
    case SPI_IO:
      return "SPI_IO";
    default:
      return "UNK_IO";
  }
}

/* connect to the output server */
static SOCKET sock_init() {
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

extern int sim_connect(sim_hal_id_t hid) {
  struct sockaddr_in addr;

  /* ignore repeated calls */
  if (simfd[hid]) {
    eprintf("%s already connected", hid2str(hid));
    return 0;
  }

  /* build addr struct */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(sim_host[hid].port);

  if (!inet_aton(sim_host[hid].ip_addr, &addr.sin_addr)) {
    eprintf("%s invalid host %s",
      hid2str(hid), sim_host[hid].ip_addr);
    exit(EXIT_FAILURE);
  }

  /* create socket and connect to remote */
  simfd[hid] = sock_init();
  if (connect(simfd[hid], (struct sockaddr*)&addr, sizeof addr)) {
    eprintf("%s connect %s:%d %s",
      hid2str(hid), sim_host[hid].ip_addr, sim_host[hid].port, strerror(errno));
    simfd[hid] = 0;
    return -1;
  }

  printf("simio %s driver connected to %s:%d\n",
    hid2str(hid), sim_host[hid].ip_addr, sim_host[hid].port);
  return 0;
}

extern ssize_t sim_read_timeout(sim_hal_id_t hid, void *buf, size_t bufsz, int timeout) {
  if (!simfd[hid]) {
    if (sim_connect(hid) < 0) {
      errno = EBADF;
      return -1;
    }
  }

  ssize_t nb = sim_preempt_read_timeout(simfd[hid], buf, bufsz, timeout);
  if (nb < 0) {
    eprintf("%s read %s", hid2str(hid), strerror(errno));
    (void)sim_disconnect(hid);
  }

  return nb;
}

extern ssize_t sim_write(sim_hal_id_t hid, void *buf, size_t bufsz) {
  if (!simfd[hid]) {
    if (sim_connect(hid) < 0) {
      errno = EBADF;
      return -1;
    }
  }

  ssize_t nb = write(simfd[hid], buf, bufsz);
  if (nb < 0) {
    eprintf("%s write %s", hid2str(hid), strerror(errno));
    (void)sim_disconnect(hid);
  }

  return nb;
}

extern int sim_printf(sim_hal_id_t hid, char *fmt, ...) {
  char buf[SIM_OUTPUT_MAX], *bufp = buf;
  int nb, left, tot = 0;
  va_list ap;

  if (!simfd[hid]) {
    if (sim_connect(hid) < 0)
      return -1;
  }

  /* fill buffer */
  va_start(ap, fmt);
  left = vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);

  /* write to socket */
  while (left) {
    if ((nb = sim_write(hid, bufp, left)) <= 0)
      break;
    left -= nb;
    bufp += nb;
    tot  += nb;
  }

  return tot;
}

extern int sim_disconnect(sim_hal_id_t hid) {
  int rv = close(simfd[hid]);
  simfd[hid] = 0;
  return rv;
}
