#include <nxu/log.h>

/*
 * Static event definitions.
 *
 * Keep this table bounded and immutable.
 */
static const struct nxu_log_event_definition
nxu_log_event_definitions[] = {

    {
        { NXU_LOG_SUBSYSTEM_BOOT,
          NXU_LOG_EVENT_BOOT_START },
        NXU_LOG_INFO,
        NXU_LOG_CRITICAL,
        0U
    },

    {
        { NXU_LOG_SUBSYSTEM_BOOT,
          NXU_LOG_EVENT_BOOT_VALIDATED },
        NXU_LOG_INFO,
        NXU_LOG_HIGH,
        0U
    },

    {
        { NXU_LOG_SUBSYSTEM_MEMORY,
          NXU_LOG_EVENT_MEMORY_REGION_DISCOVERED },
        NXU_LOG_INFO,
        NXU_LOG_NORMAL,
        24U
    },

    {
        { NXU_LOG_SUBSYSTEM_CPU,
          NXU_LOG_EVENT_CPU_ONLINE },
        NXU_LOG_INFO,
        NXU_LOG_HIGH,
        4U
    },

    {
        { NXU_LOG_SUBSYSTEM_CPU,
          NXU_LOG_EVENT_CPU_START_FAILED },
        NXU_LOG_ERROR,
        NXU_LOG_CRITICAL,
        8U
    },

    {
        { NXU_LOG_SUBSYSTEM_INTERRUPT,
          NXU_LOG_EVENT_INTERRUPT_DISPATCH },
        NXU_LOG_DEBUG,
        NXU_LOG_LOW,
        8U
    },

    {
        { NXU_LOG_SUBSYSTEM_INTERRUPT,
          NXU_LOG_EVENT_INTERRUPT_ERROR },
        NXU_LOG_ERROR,
        NXU_LOG_CRITICAL,
        8U
    },

    {
        { NXU_LOG_SUBSYSTEM_CONSOLE,
          NXU_LOG_EVENT_CONSOLE_ERROR },
        NXU_LOG_ERROR,
        NXU_LOG_HIGH,
        4U
    },

    {
        { NXU_LOG_SUBSYSTEM_KERNEL,
          NXU_LOG_EVENT_KERNEL_ERROR },
        NXU_LOG_ERROR,
        NXU_LOG_CRITICAL,
        32U
    },

    {
        { NXU_LOG_SUBSYSTEM_KERNEL,
          NXU_LOG_EVENT_KERNEL_PANIC },
        NXU_LOG_FATAL,
        NXU_LOG_CRITICAL,
        32U
    }
};

#define NXU_LOG_EVENT_DEFINITION_COUNT \
    (sizeof(nxu_log_event_definitions) / \
     sizeof(nxu_log_event_definitions[0]))

const struct nxu_log_event_definition *
nxu_log_event_definition_get(
    const struct nxu_log_event *event
)
{
    nxu_u32 i;

    if (!event)
        return 0;

    for (i = 0U;
         i < (nxu_u32)NXU_LOG_EVENT_DEFINITION_COUNT;
         i++) {

        if (nxu_log_event_definitions[i].event.subsystem ==
                event->subsystem &&
            nxu_log_event_definitions[i].event.code ==
                event->code) {

            return &nxu_log_event_definitions[i];
        }
    }

    return 0;
}