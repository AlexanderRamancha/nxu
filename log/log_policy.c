#include <nxu/log.h>

/*
 * NXU Logging Policy
 *
 * Policy answers one question:
 *
 *   "Given this event and the execution context,
 *    what logging characteristics are permitted?"
 *
 * Policy does not:
 *   - allocate memory
 *   - access hardware
 *   - write storage
 *   - access the console
 *   - block
 */

/*
 * PANIC is the only context permitted to use the
 * emergency / protected diagnostic path in v0.1.
 */
int
nxu_log_policy_allow_emergency(
    nxu_u8 context
)
{
    return context == NXU_LOG_CONTEXT_PANIC;
}

/*
 * Determine whether the current execution context
 * permits the normal logging path.
 *
 * NORMAL:
 *   Fully permitted.
 *
 * IRQ:
 *   Permitted only because the eventual storage path
 *   is required to be bounded and non-blocking.
 *
 * EXCEPTION:
 *   Normal path is rejected initially. Exception
 *   diagnostics use the restricted/emergency path.
 *
 * PANIC:
 *   Normal infrastructure is not trusted.
 */
int
nxu_log_policy_allow_normal(
    nxu_u8 context
)
{
    switch (context) {

    case NXU_LOG_CONTEXT_NORMAL:
        return 1;

    case NXU_LOG_CONTEXT_IRQ:
        return 1;

    case NXU_LOG_CONTEXT_EXCEPTION:
        return 0;

    case NXU_LOG_CONTEXT_PANIC:
        return 0;

    default:
        return 0;
    }
}

/*
 * Determine whether an event may enter the normal
 * bounded diagnostic storage path.
 *
 * Delivery priority is intentionally not used here
 * to override execution-context safety.
 *
 * A CRITICAL event in an unsafe context must not
 * bypass the context restrictions.
 */
int
nxu_log_policy_allow_normal_priority(
    nxu_u8 context,
    nxu_u8 priority
)
{
    (void)priority;

    return nxu_log_policy_allow_normal(context);
}