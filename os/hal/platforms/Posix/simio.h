#ifndef SIMIO_H
#define SIMIO_H

#include <stdio.h>
#include "ch.h"

/* data buffer size for sim_printf */
#define SIM_OUTPUT_MAX 1024

/* index to array of IO ports - order must match elements in simio.c sim_host[] */
typedef enum {
  PAL_IO,
  SD1_IO,
  SD2_IO,
  EXT_IO,
} sim_hal_id_t;

typedef void (*simio_cb_t)(char *buf, void *arg);

/* configure */
extern void sim_getopt(int argc, char **argv);

/* startup */
extern int sim_connect(sim_hal_id_t hid);

/* read data sent to the HAL */
extern ssize_t sim_read_timeout(sim_hal_id_t hid, void *buf, size_t bufsz, int timeout);
#define sim_read(a,b,c) sim_read_timeout((a),(b),(c),TIME_INFINITE)

/* write data from the HAL */
extern ssize_t sim_write(sim_hal_id_t hid, void *buf, size_t bufsz);
extern int sim_printf(sim_hal_id_t hid, char *fmt, ...);

/* shutdown */
extern int sim_disconnect(sim_hal_id_t hid);

#endif /* SIMIO_H */
