#ifndef SIMIO_H
#define SIMIO_H

#define SIM_INPUT_MAX 80

typedef void (*simio_cb_t)(char*, void*);

void sim_io_init(uint32_t porti, uint32_t porto);

void sim_input_cb(simio_cb_t cb, void *arg);
void sim_output_puts(unsigned char *buf, uint32_t size);

#endif /* SIMIO_H */
