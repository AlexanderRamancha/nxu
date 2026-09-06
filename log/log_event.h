#ifndef NXU_LOG_EVENT_H
#define NXU_LOG_EVENT_H

#include <nxu/log.h>

const struct nxu_log_event_definition *
nxu_log_event_definition_get(
    const struct nxu_log_event *event
);

#endif