/*
 * NXU interrupt manager
 *
 * The manager is the sole architectural owner of interrupt state.
 * Subsystems receive opaque interrupt references and handler context.
 * The backend is invoked only by this manager.
 */

#include <nxu/interrupt_manager.h>
#include <nxu/interrupt_internal.h>
#include <nxu/interrupt_backend.h>
#include <nxu/cpu.h>

#define NXU_MAX_INTERRUPTS 1024U
#define NXU_INTERRUPT_SPECIAL 1020U

static struct nxu_interrupt interrupt_table[NXU_MAX_INTERRUPTS];
static nxu_u32 registered_limit;

void
nxu_interrupt_manager_init(nxu_u32 interrupt_count)
{
    nxu_u32 i;

    if (interrupt_count > NXU_MAX_INTERRUPTS)
        interrupt_count = NXU_MAX_INTERRUPTS;

    registered_limit = interrupt_count;

    for (i = 0U; i < NXU_MAX_INTERRUPTS; i++) {
        interrupt_table[i].allocated = 0U;
        interrupt_table[i].handler = 0;
        interrupt_table[i].handler_context = 0;
        interrupt_table[i].state = NXU_INTERRUPT_DISABLED;
    }
}

int
nxu_interrupt_create(
    nxu_u32 intid,
    enum nxu_interrupt_type type,
    nxu_interrupt_handler handler,
    void *context,
    struct nxu_interrupt **out
)
{
    struct nxu_interrupt *interrupt;

    if (!out || !handler)
        return -1;

    if (intid >= registered_limit)
        return -1;

    interrupt = &interrupt_table[intid];

    if (interrupt->allocated)
        return -1;

    interrupt->intid = intid;
    interrupt->type = type;
    interrupt->trigger =
        (type == NXU_INTERRUPT_SGI)
            ? NXU_INTERRUPT_EDGE
            : NXU_INTERRUPT_LEVEL;
    interrupt->priority = 0U;
    interrupt->target_cpu = 0U;
    interrupt->state = NXU_INTERRUPT_DISABLED;
    interrupt->handler = handler;
    interrupt->handler_context = context;
    interrupt->allocated = 1U;

    *out = interrupt;

    return 0;
}

static int
nxu_interrupt_valid(struct nxu_interrupt *interrupt)
{
    if (!interrupt)
        return 0;

    if (interrupt < &interrupt_table[0] ||
        interrupt >= &interrupt_table[NXU_MAX_INTERRUPTS])
        return 0;

    if (!interrupt->allocated)
        return 0;

    if (interrupt->intid >= registered_limit)
        return 0;

    if (&interrupt_table[interrupt->intid] != interrupt)
        return 0;

    return 1;
}

int
nxu_interrupt_configure(
    struct nxu_interrupt *interrupt,
    const struct nxu_interrupt_config *config
)
{
    const struct nxu_interrupt_backend *backend;
    int result;

    if (!nxu_interrupt_valid(interrupt) || !config)
        return -1;

    if (interrupt->state == NXU_INTERRUPT_ENABLED)
        return -1;

    backend = nxu_interrupt_backend_get();

    if (!backend || !backend->configure)
        return -1;

    result = backend->configure(interrupt, config);

    if (result != 0)
        return result;

    interrupt->priority = config->priority;
    interrupt->trigger = config->trigger;
    interrupt->target_cpu = config->target_cpu;

    return 0;
}

int
nxu_interrupt_enable(struct nxu_interrupt *interrupt)
{
    const struct nxu_interrupt_backend *backend;
    int result;

    if (!nxu_interrupt_valid(interrupt))
        return -1;

    if (interrupt->state == NXU_INTERRUPT_ENABLED)
        return 0;

    backend = nxu_interrupt_backend_get();

    if (!backend || !backend->enable)
        return -1;

    result = backend->enable(interrupt);

    if (result != 0)
        return result;

    interrupt->state = NXU_INTERRUPT_ENABLED;

    return 0;
}

int
nxu_interrupt_disable(struct nxu_interrupt *interrupt)
{
    const struct nxu_interrupt_backend *backend;
    int result;

    if (!nxu_interrupt_valid(interrupt))
        return -1;

    if (interrupt->state == NXU_INTERRUPT_DISABLED)
        return 0;

    backend = nxu_interrupt_backend_get();

    if (!backend || !backend->disable)
        return -1;

    result = backend->disable(interrupt);

    if (result != 0)
        return result;

    interrupt->state = NXU_INTERRUPT_DISABLED;

    return 0;
}

static int
nxu_interrupt_dispatch(nxu_u32 intid)
{
    struct nxu_interrupt *interrupt;
    struct nxu_cpu *cpu;
    struct nxu_interrupt_cpu_context *context;
    struct nxu_interrupt_context_frame *frame;
    nxu_u32 cpu_id;
    nxu_u32 depth;

    if (intid >= registered_limit || intid >= NXU_INTERRUPT_SPECIAL)
        return -1;

    interrupt = &interrupt_table[intid];

    if (!interrupt->allocated)
        return -1;

    if (interrupt->state != NXU_INTERRUPT_ENABLED)
        return -1;

    if (!interrupt->handler)
        return -1;

    cpu_id = nxu_cpu_current_id();

    if (cpu_id >= NXU_MAX_CPUS)
        return -1;

    cpu = nxu_cpu_get(cpu_id);

    if (!cpu)
        return -1;

    context = &cpu->interrupt_context;
    depth = context->depth;

    if (depth >= NXU_MAX_INTERRUPT_NESTING)
        return -1;

    frame = &context->frames[depth];
    frame->active = 1U;
    frame->intid = interrupt->intid;
    frame->priority = interrupt->priority;
    context->depth = depth + 1U;

    /*
     * The handler receives only its context. The interrupt object
     * never crosses the dispatch boundary.
     */
    interrupt->handler(interrupt->handler_context);

    frame->active = 0U;
    frame->intid = 0U;
    frame->priority = 0U;
    context->depth = depth;

    return 0;
}

int
nxu_interrupt_handle(void)
{
    const struct nxu_interrupt_backend *backend;
    nxu_u32 intid;

    backend = nxu_interrupt_backend_get();

    if (!backend || !backend->acknowledge || !backend->complete)
        return -1;

    /*
     * The backend only identifies the physical event.
     * It does not decide what that event is allowed to cause.
     */
    intid = backend->acknowledge();

    if (intid >= NXU_INTERRUPT_SPECIAL) {
        backend->complete(intid);
        return 1;
    }

    /*
     * Unknown, disabled, or unbound events are consumed at the
     * manager boundary. They never reach a subsystem handler.
     */
    if (nxu_interrupt_dispatch(intid) != 0) {
        backend->complete(intid);
        return 0;
    }

    backend->complete(intid);

    return 0;
}

nxu_u32
nxu_interrupt_nesting_depth(void)
{
    struct nxu_cpu *cpu;
    nxu_u32 cpu_id = nxu_cpu_current_id();

    if (cpu_id >= NXU_MAX_CPUS)
        return 0U;

    cpu = nxu_cpu_get(cpu_id);

    if (!cpu)
        return 0U;

    return cpu->interrupt_context.depth;
}

nxu_u32
nxu_interrupt_current_intid(void)
{
    struct nxu_cpu *cpu;
    struct nxu_interrupt_cpu_context *context;
    nxu_u32 cpu_id = nxu_cpu_current_id();
    nxu_u32 depth;

    if (cpu_id >= NXU_MAX_CPUS)
        return 0U;

    cpu = nxu_cpu_get(cpu_id);

    if (!cpu)
        return 0U;

    context = &cpu->interrupt_context;
    depth = context->depth;

    if (depth == 0U)
        return 0U;

    return context->frames[depth - 1U].intid;
}

nxu_u8
nxu_interrupt_current_priority(void)
{
    struct nxu_cpu *cpu;
    struct nxu_interrupt_cpu_context *context;
    nxu_u32 cpu_id = nxu_cpu_current_id();
    nxu_u32 depth;

    if (cpu_id >= NXU_MAX_CPUS)
        return 0U;

    cpu = nxu_cpu_get(cpu_id);

    if (!cpu)
        return 0U;

    context = &cpu->interrupt_context;
    depth = context->depth;

    if (depth == 0U)
        return 0U;

    return context->frames[depth - 1U].priority;
}

int
nxu_interrupt_in_context(void)
{
    return nxu_interrupt_nesting_depth() != 0U;
}
