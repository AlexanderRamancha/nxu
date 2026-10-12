/*
 * NXU MAP
 * Layer: foundation observe
 * Owns: filling a log record from a semantic event
 * Consumes: event id and values
 * Produces: chronicle slot, then console presentation
 * Hardware: none
 * Forbidden: timer read; allocation; failure because the sink is down
 * Invariant: returns without changing EL, MMU, GIC, or CPU life
 */

#include <nxu/log.h>
#include <nxu/chronicle.h>
#include <nxu/console.h>
#include <nxu/cpu.h>
#include <nxu/config.h>

void
nxu_log(nxu_u32 event, nxu_u32 severity, nxu_u32 subsystem,
    nxu_u64 v0, nxu_u64 v1, nxu_u64 v2,
    const char *func, const char *file, nxu_u32 line)
{
	struct nxu_log_record r;
	nxu_u32 cpu;

	cpu = nxu_this_logical();
	if (cpu >= NXU_CPU_BOUND)
		cpu = 0U;

	r.event = event;
	r.severity = severity;
	r.subsystem = subsystem;
	r.cpu = cpu;
	r.v0 = v0;
	r.v1 = v1;
	r.v2 = v2;
	r.func = func;
	r.file = file;
	r.line = line;

	nxu_chronicle_put(event, v0, v1);
	nxu_console_record(&r);
}
