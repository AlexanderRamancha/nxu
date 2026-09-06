#ifndef NXU_LOG_CPU_H
#define NXU_LOG_CPU_H

#include <nxu/log.h>

void
nxu_log_cpu_init(void);

struct nxu_log_cpu_state *
nxu_log_cpu_current(void);

nxu_u32
nxu_log_cpu_current_id(void);

nxu_u8
nxu_log_cpu_context(void);

#endif