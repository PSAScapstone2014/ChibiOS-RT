#ifndef SIM_PREEMPT_H
#define SIM_PREEMPT_H

#include "ch.h"

#ifdef SIM_PREEMPT_REPLACE_FUNCTIONS
# define ACCEPT(a,b,c) sim_preempt_accept((a),(b),(c))
# define READ(a,b,c) sim_preempt_read((a),(b),(c))
#else
# define ACCEPT(a,b,c) accept((a), (b), (c))
# define READ(a,b,c) read((a),(b),(c))
#endif /* SIM_PREEMPT_REPLACE_FUNCTIONS */

extern int sim_preempt_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern size_t sim_preempt_read_timeout(int fd, void *buf, size_t bufsz, systime_t timeout);

#define sim_preempt_read(a,b,c) sim_preempt_read_timeout((a),(b),(c),TIME_INFINITE)

#endif /* SIM_PREEMPT_H */
