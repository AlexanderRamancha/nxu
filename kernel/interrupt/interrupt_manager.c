#include <nxu/interrupt_manager.h>
#include <nxu/interrupt_backend.h>
#include <nxu/cpu.h>

#define NXU_MAX_INTERRUPTS 1024U

static struct nxu_interrupt *
interrupt_table[NXU_MAX_INTERRUPTS];

static nxu_u32
registered_limit;


void
nxu_interrupt_manager_init(
    nxu_u32 interrupt_count
)
{
    nxu_u32 i;

    if (interrupt_count > NXU_MAX_INTERRUPTS)
        interrupt_count =
            NXU_MAX_INTERRUPTS;

    registered_limit =
        interrupt_count;

    for (
        i = 0U;
        i < NXU_MAX_INTERRUPTS;
        i++
    )
        interrupt_table[i] = 0;
}


int
nxu_interrupt_register(
    struct nxu_interrupt *interrupt
)
{
    if (interrupt == 0)
        return -1;

    if (
        interrupt->intid >=
        registered_limit
    )
        return -1;

    if (
        interrupt_table[
            interrupt->intid
        ] != 0
    )
        return -1;

    interrupt_table[
        interrupt->intid
    ] = interrupt;

    return 0;
}


struct nxu_interrupt *
nxu_interrupt_lookup(
    nxu_u32 intid
)
{
    if (
        intid >=
        registered_limit
    )
        return 0;

    return interrupt_table[intid];
}


int
nxu_interrupt_configure(
    struct nxu_interrupt *interrupt,
    const struct nxu_interrupt_config *config
)
{
    const struct nxu_interrupt_backend *backend;
    int result;

    if (
        interrupt == 0 ||
        config == 0
    )
        return -1;

    if (
        interrupt->state ==
        NXU_INTERRUPT_ENABLED
    )
        return -1;

    backend =
        nxu_interrupt_backend_get();

    if (
        backend == 0 ||
        backend->configure == 0
    )
        return -1;

    result =
        backend->configure(
            interrupt,
            config
        );

    if (result != 0)
        return result;

    interrupt->priority =
        config->priority;

    interrupt->trigger =
        config->trigger;

    interrupt->target_cpu =
        config->target_cpu;

    return 0;
}


int
nxu_interrupt_enable(
    struct nxu_interrupt *interrupt
)
{
    const struct nxu_interrupt_backend *backend;
    int result;

    if (interrupt == 0)
        return -1;

    if (
        interrupt->state ==
        NXU_INTERRUPT_ENABLED
    )
        return 0;

    backend =
        nxu_interrupt_backend_get();

    if (
        backend == 0 ||
        backend->enable == 0
    )
        return -1;

    result =
        backend->enable(
            interrupt
        );

    if (result != 0)
        return result;

    interrupt->state =
        NXU_INTERRUPT_ENABLED;

    return 0;
}


int
nxu_interrupt_disable(
    struct nxu_interrupt *interrupt
)
{
    const struct nxu_interrupt_backend *backend;
    int result;

    if (interrupt == 0)
        return -1;

    if (
        interrupt->state ==
        NXU_INTERRUPT_DISABLED
    )
        return 0;

    backend =
        nxu_interrupt_backend_get();

    if (
        backend == 0 ||
        backend->disable == 0
    )
        return -1;

    result =
        backend->disable(
            interrupt
        );

    if (result != 0)
        return result;

    interrupt->state =
        NXU_INTERRUPT_DISABLED;

    return 0;
}


int
nxu_interrupt_set_handler(
    struct nxu_interrupt *interrupt,
    void (*handler)(
        struct nxu_interrupt *interrupt
    ),
    void *context
)
{
    if (
        interrupt == 0 ||
        handler == 0
    )
        return -1;

    if (
        interrupt->state ==
        NXU_INTERRUPT_ENABLED
    )
        return -1;

    interrupt->handler =
        handler;

    interrupt->handler_context =
        context;

    return 0;
}


int
nxu_interrupt_dispatch(
    nxu_u32 intid
)
{
    struct nxu_interrupt *interrupt;
    struct nxu_cpu *cpu;
    struct nxu_interrupt_cpu_context *context;
    struct nxu_interrupt_context_frame *frame;
    nxu_u32 cpu_id;
    nxu_u32 depth;

    interrupt =
        nxu_interrupt_lookup(
            intid
        );

    if (interrupt == 0)
        return -1;

    if (
        interrupt->state !=
        NXU_INTERRUPT_ENABLED
    )
        return -1;

    if (interrupt->handler == 0)
        return -1;

    cpu_id =
        nxu_cpu_current_id();

    if (
        cpu_id >=
        NXU_MAX_CPUS
    )
        return -1;

    cpu =
        nxu_cpu_get(
            cpu_id
        );

    if (cpu == 0)
        return -1;

    context =
        &cpu->interrupt_context;

    depth =
        context->depth;

    if (
        depth >=
        NXU_MAX_INTERRUPT_NESTING
    )
        return -1;

    frame =
        &context->frames[
            depth
        ];

    frame->active =
        1U;

    frame->intid =
        interrupt->intid;

    frame->priority =
        interrupt->priority;

    context->depth =
        depth + 1U;

    interrupt->handler(
        interrupt
    );

    frame->active =
        0U;

    frame->intid =
        0U;

    frame->priority =
        0U;

    context->depth =
        depth;

    return 0;
}


nxu_u32
nxu_interrupt_nesting_depth(void)
{
    struct nxu_cpu *cpu;
    nxu_u32 cpu_id;

    cpu_id =
        nxu_cpu_current_id();

    if (
        cpu_id >=
        NXU_MAX_CPUS
    )
        return 0U;

    cpu =
        nxu_cpu_get(
            cpu_id
        );

    if (cpu == 0)
        return 0U;

    return cpu->interrupt_context.depth;
}


nxu_u32
nxu_interrupt_current_intid(void)
{
    struct nxu_cpu *cpu;
    struct nxu_interrupt_cpu_context *context;
    nxu_u32 cpu_id;
    nxu_u32 depth;

    cpu_id =
        nxu_cpu_current_id();

    if (
        cpu_id >=
        NXU_MAX_CPUS
    )
        return 0U;

    cpu =
        nxu_cpu_get(
            cpu_id
        );

    if (cpu == 0)
        return 0U;

    context =
        &cpu->interrupt_context;

    depth =
        context->depth;

    if (depth == 0U)
        return 0U;

    return context->frames[
        depth - 1U
    ].intid;
}


nxu_u8
nxu_interrupt_current_priority(void)
{
    struct nxu_cpu *cpu;
    struct nxu_interrupt_cpu_context *context;
    nxu_u32 cpu_id;
    nxu_u32 depth;

    cpu_id =
        nxu_cpu_current_id();

    if (
        cpu_id >=
        NXU_MAX_CPUS
    )
        return 0U;

    cpu =
        nxu_cpu_get(
            cpu_id
        );

    if (cpu == 0)
        return 0U;

    context =
        &cpu->interrupt_context;

    depth =
        context->depth;

    if (depth == 0U)
        return 0U;

    return context->frames[
        depth - 1U
    ].priority;
}


int
nxu_interrupt_in_context(void)
{
    return
        nxu_interrupt_nesting_depth() != 0U;
}