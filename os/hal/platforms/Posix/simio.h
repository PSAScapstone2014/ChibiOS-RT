#ifndef SIMIO_H
#define SIMIO_H

/* data buffer sizes for sim_printf and sim_set_input_cb */
#define SIM_INPUT_MAX 80
#define SIM_OUTPUT_MAX 256

/* index to array of IO ports - order must match elements in simio.c sim_host[] */
typedef enum {
  PAL_OUT,
  SD1_OUT,
  SD2_OUT,
  EXT_INT,
  SD1_IN,
  SD2_IN,
} sim_hal_id_t;

typedef void (*simio_cb_t)(char *buf, void *arg);

/* configure */
extern void sim_getopt(int argc, char **argv);

/* startup */
extern void sim_accept_input(sim_hal_id_t);
extern void sim_connect_output(sim_hal_id_t hid);

/* read data - do not mix these calls on the same HAL ID */
extern ssize_t sim_read(sim_hal_id_t hid, void *buf, size_t bufsz);
extern void sim_set_input_cb(sim_hal_id_t hid, simio_cb_t cb, void *arg);

/* write data to the given HAL ID */
extern ssize_t sim_write(sim_hal_id_t hid, void *buf, size_t bufsz);
extern void sim_printf(sim_hal_id_t hid, char *fmt, ...);

/* shutdown */
extern void sim_io_stop(void);

/* locks around irqs */
extern void sim_io_lock(void);
extern void sim_io_unlock(void);

#endif /* SIMIO_H */
