#include <nxu/log.h>

#include "log_storage.h"
#include "log_event.h"
#include "log_cpu.h"
#include "log_policy.h"

int
nxu_log_event(
    const struct nxu_log_event *event,
    const void *data,
    nxu_u8 data_size
)
{
    const struct nxu_log_event_definition *definition;
    struct nxu_log_record *record;
    nxu_u32 cpu_id;
    nxu_u8 context;

    if (!event)
        return -1;

    if (data_size > NXU_LOG_DATA_SIZE)
        return -1;

    if (data_size != 0U && !data)
        return -1;

    definition = nxu_log_event_definition_get(event);

    if (!definition)
        return -1;

    if (data_size > definition->payload_size)
    return -1;

    context = nxu_log_cpu_context();

    if (!nxu_log_policy_allow_normal_priority(
            context,
            definition->priority))
        return -1;

    record = nxu_log_storage_reserve();

    if (!record)
        return -1;

    cpu_id = nxu_log_cpu_current_id();

    record->event = *event;
    record->severity = definition->severity;
    record->priority = definition->priority;
    record->context = context;
    record->reserved = 0U;
    record->cpu = cpu_id;

    record->data.size = data_size;

    if (data_size != 0U) {
        nxu_u8 i;

        for (i = 0U; i < data_size; i++)
            record->data.bytes[i] =
                ((const nxu_u8 *)data)[i];
    }

    if (nxu_log_storage_commit() != 0)
        return -1;

    return 0;
}

void
nxu_log_init(void)
{
    nxu_log_cpu_init();
    nxu_log_storage_init();
}