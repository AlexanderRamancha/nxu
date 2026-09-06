#ifndef NXU_LOG_POLICY_H
#define NXU_LOG_POLICY_H

#include <nxu/log.h>

int
nxu_log_policy_allow_emergency(
    nxu_u8 context
);

int
nxu_log_policy_allow_normal(
    nxu_u8 context
);

int
nxu_log_policy_allow_normal_priority(
    nxu_u8 context,
    nxu_u8 priority
);

#endif