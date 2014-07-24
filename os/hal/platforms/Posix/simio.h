#ifndef SIMIO_H
#define SIMIO_H

#define SIM_INPUT_MAX 80
#define SIM_OUTPUT_MAX 256

typedef void (*simio_cb_t)(char*, void*);

void sim_accept_input(long port);
void sim_connect_output(char *host, uint16_t port);

void sim_set_input_cb(simio_cb_t cb, void *arg);
void sim_printf(char *fmt, ...);

void sim_io_stop(void);

void sim_getopt(int argc, char **argv);

#endif /* SIMIO_H */
