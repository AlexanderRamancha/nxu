#ifndef NXU_LOG_H
#define NXU_LOG_H

#include <nxu/types.h>

/*
 * NXU Logging
 *
 * Event
 *   Identifies what happened.
 *
 * Severity
 *   Describes how serious the event is.
 *
 * Priority
 *   Describes how strongly the diagnostic record
 *   should be protected from dropping.
 *
 * Context
 *   Describes the execution environment in which
 *   the event was captured.
 *
 * Event data
 *   Bounded, trivially-copyable event-specific value data.
 *
 * Log record
 *   Canonical diagnostic unit captured by the Logger.
 */

enum nxu_log_severity {
    NXU_LOG_FATAL = 0,
    NXU_LOG_ERROR,
    NXU_LOG_WARN,
    NXU_LOG_INFO,
    NXU_LOG_DEBUG,
    NXU_LOG_TRACE
};

enum nxu_log_priority {
    NXU_LOG_CRITICAL = 0,
    NXU_LOG_HIGH,
    NXU_LOG_NORMAL,
    NXU_LOG_LOW
};

enum nxu_log_context {
    NXU_LOG_CONTEXT_NORMAL = 0,
    NXU_LOG_CONTEXT_IRQ,
    NXU_LOG_CONTEXT_EXCEPTION,
    NXU_LOG_CONTEXT_PANIC
};

enum nxu_log_subsystem {
    NXU_LOG_SUBSYSTEM_BOOT = 1,
    NXU_LOG_SUBSYSTEM_MEMORY,
    NXU_LOG_SUBSYSTEM_CPU,
    NXU_LOG_SUBSYSTEM_INTERRUPT,
    NXU_LOG_SUBSYSTEM_CONSOLE,
    NXU_LOG_SUBSYSTEM_KERNEL
};

enum nxu_log_event_code {
    NXU_LOG_EVENT_BOOT_START = 1,
    NXU_LOG_EVENT_BOOT_VALIDATED,

    NXU_LOG_EVENT_MEMORY_REGION_DISCOVERED,

    NXU_LOG_EVENT_CPU_ONLINE,
    NXU_LOG_EVENT_CPU_START_FAILED,

    NXU_LOG_EVENT_INTERRUPT_DISPATCH,
    NXU_LOG_EVENT_INTERRUPT_ERROR,

    NXU_LOG_EVENT_CONSOLE_ERROR,

    NXU_LOG_EVENT_KERNEL_ERROR,
    NXU_LOG_EVENT_KERNEL_PANIC
};

/*
 * Event identity.
 *
 * The subsystem owns the meaning of the event value.
 * Logger only transports/interprets the common identity.
 */
struct nxu_log_event {
    nxu_u16 subsystem;
    nxu_u16 code;
};

/*
 * Bounded event-specific payload.
 *
 * The payload is copied into the canonical record.
 * No caller-owned pointer is retained.
 */
#define NXU_LOG_DATA_SIZE 32U

struct nxu_log_data {
    nxu_u8 bytes[NXU_LOG_DATA_SIZE];
    nxu_u8 size;
};

/*
 * Canonical diagnostic record.
 *
 * CPU and context are derived by Logger.
 * Severity and priority are selected by event policy.
 */
struct nxu_log_record {
    struct nxu_log_event event;

    nxu_u8 severity;
    nxu_u8 priority;
    nxu_u8 context;
    nxu_u8 reserved;

    nxu_u32 cpu;

    struct nxu_log_data data;
};

/*
 * Logger lifecycle.
 */
void
nxu_log_init(void);

/*
 * Capture a bounded diagnostic event.
 *
 * The caller supplies event identity and bounded value data.
 * Logger derives CPU/context and applies event policy.
 *
 * Returns:
 *   0  - accepted by the normal logging path
 *  -1  - not accepted
 */
int
nxu_log_event(
    const struct nxu_log_event *event,
    const void *data,
    nxu_u8 data_size
);

struct nxu_log_event_definition {
    struct nxu_log_event event;
    nxu_u8 severity;
    nxu_u8 priority;
    nxu_u8 payload_size;
};


#endif