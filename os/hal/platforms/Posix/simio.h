#ifndef SIMIO_H
#define SIMIO_H

#define SIM_INPUT_MAX 80
#define SIM_OUTPUT_MAX 256

/* index to array of IO ports */
typedef enum {
  HAL_OUT,
  HAL_EXT
} sim_hal_id_t;

typedef void (*simio_cb_t)(char *buf, void *arg);

/* configure */
extern void sim_getopt(int argc, char **argv);

/* startup */
extern void sim_connect_output(void);
extern void sim_set_input_cb(sim_hal_id_t hid, simio_cb_t cb, void *arg);

/* write messages to server */
extern void sim_printf(char *fmt, ...);

/* shutdown */
extern void sim_io_stop(void);

/* locks around irqs */
extern void sim_io_lock(void);
extern void sim_io_unlock(void);

#endif /* SIMIO_H */
