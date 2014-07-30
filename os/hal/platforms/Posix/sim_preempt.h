#ifndef SIM_PREEMPT_H
#define SIM_PREEMPT_H

#include "ch.h"

// #ifdef SIM_REPLACE_FUNCTIONS_PREEMPT
// # define accept(a, b, c) sim_accept((a), (b), (c))
// # define read(a, b, c) sim_read((a), (b), (c))
// #endif /* SIM_REPLACE_FUNCTIONS_PREEMPT */

extern int sim_preempt_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern size_t sim_preempt_read_timeout(int fd, void *buf, size_t bufsz, systime_t timeout);

#define sim_preempt_read(a,b,c) sim_preempt_read_timeout((a),(b),(c),TIME_INFINITE)

#endif /* SIM_PREEMPT_H */
