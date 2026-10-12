/*
 * NXU MAP
 *
 *                    failure
 *                       │
 *                       ▼
 *                  log record
 *                       │
 *                       ▼
 *                 black box dump
 *                       │
 *                       ▼
 *                     halt
 *
 * Layer: foundation observe
 * Owns: the halt transition
 * Consumes: failure code and the call site
 * Produces: immortal failure slot, one fail line, dump, halt
 * Hardware: none
 * Forbidden: continue after a broken invariant
 * Invariant: location is the call site, not a guessed string
 */

#include <nxu/fail.h>
#include <nxu/chronicle.h>
#include <nxu/log.h>
#include <nxu/bbox.h>
#include <nxu/halt.h>

void
nxu_fail_at(nxu_u32 code, nxu_u64 a, nxu_u64 b,
    const char *func, const char *file, nxu_u32 line)
{
	struct nxu_chronicle *c;

	c = nxu_chronicle_this();
	c->fail_func = func;
	c->fail_file = file;
	c->fail_line = line;
	nxu_chronicle_fail(code, a, b);
	nxu_log(NXU_EV_FAIL, NXU_SEV_FAIL, NXU_SUB_FAIL, code, a, b,
	    func, file, line);
	nxu_bbox_dump();
	nxu_halt();
}
