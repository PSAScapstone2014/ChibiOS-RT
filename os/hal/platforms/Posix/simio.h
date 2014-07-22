#ifndef SIMIO_H
#define SIMIO_H

#define SIM_INPUT_MAX 80
#define SIM_OUTPUT_TXTSZ 256

typedef void (*simio_cb_t)(char*, void*);

void sim_io_start(void);
void sim_io_stop(void);

void sim_getopt(int argc, char **argv);
void sim_sdStart(void);
void sim_sdStop(void);

void sim_input_cb(simio_cb_t cb, void *arg);
void sim_printf(char *fmt, ...);

#endif /* SIMIO_H */
