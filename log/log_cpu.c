#include <nxu/log.h>
#include <nxu/cpu.h>
#include <nxu/interrupt_manager.h>

struct nxu_log_cpu_state {
    nxu_u32 logical_id;
};

/*
 * Per-CPU logging state.
 *
 * This is indexed by logical CPU ID.
 * A CPU may use only its own entry while producing
 * a diagnostic record.
 */
static struct nxu_log_cpu_state
nxu_log_cpu_states[NXU_MAX_CPUS];

void
nxu_log_cpu_init(void)
{
    nxu_u32 cpu_id;

    for (cpu_id = 0U;
         cpu_id < NXU_MAX_CPUS;
         cpu_id++) {

        nxu_log_cpu_states[cpu_id].logical_id = cpu_id;
    }
}

nxu_u32
nxu_log_cpu_current_id(void)
{
    return nxu_cpu_current_id();
}

nxu_u8
nxu_log_cpu_context(void)
{
    /*
     * Existing NXU interrupt infrastructure already
     * provides the authoritative per-CPU interrupt state.
     *
     * No independent Logger context state is maintained.
     */
    if (nxu_interrupt_in_context())
        return NXU_LOG_CONTEXT_IRQ;

    return NXU_LOG_CONTEXT_NORMAL;
}