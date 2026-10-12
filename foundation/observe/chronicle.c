/*
 * NXU MAP
 *
 *                    FOUNDATION OBSERVE
 *                           │
 *                 ┌─────────┴─────────┐
 *                 ▼                   ▼
 *             CHRONICLE            BLACK BOX
 *                 │
 *                 ▼
 *              CONSOLE
 *
 * Layer: foundation observe
 * Owns: per-CPU append-only chronicle + immortal last-failure slot
 * Consumes: CPU logical id; event code and two values
 * Produces: records of transitions; copy into Black Box
 * Hardware: none
 * Forbidden: logging framework; fail-open on overflow
 * Invariant: overflow never fail-opens
 */

#include <nxu/chronicle.h>
#include <nxu/cpu.h>
#include <nxu/bbox.h>

struct nxu_chronicle nxu_cpu_chronicle[NXU_CPU_BOUND];

struct nxu_chronicle *
nxu_chronicle_this(void)
{
	nxu_u32 id;

	id = nxu_this_logical();
	if (id >= NXU_CPU_BOUND)
		id = 0U;
	return &nxu_cpu_chronicle[id];
}

void
nxu_chronicle_init(void)
{
	struct nxu_chronicle *c;
	nxu_u32 i;
	nxu_u32 cpu;

	for (cpu = 0U; cpu < NXU_CPU_BOUND; cpu++) {
		c = &nxu_cpu_chronicle[cpu];
		c->next = 0U;
		c->count = 0U;
		c->fail_code = 0U;
		c->fail_a = 0U;
		c->fail_b = 0U;
		c->fail_func = NXU_NULL;
		c->fail_file = NXU_NULL;
		c->fail_line = 0U;
		c->fail_valid = 0U;
		for (i = 0U; i < NXU_CHRONICLE_CAP; i++) {
			c->rec[i].seq = 0U;
			c->rec[i].code = 0U;
			c->rec[i].a = 0U;
			c->rec[i].b = 0U;
		}
	}
	nxu_bbox_init();
}

void
nxu_chronicle_put(nxu_u32 code, nxu_u64 a, nxu_u64 b)
{
	struct nxu_chronicle *c;
	struct nxu_chronicle_record *r;
	nxu_u32 i;

	c = nxu_chronicle_this();
	i = c->next % NXU_CHRONICLE_CAP;
	r = &c->rec[i];
	r->seq = c->count;
	r->code = code;
	r->a = a;
	r->b = b;
	c->next = i + 1U;
	c->count++;
	nxu_bbox_put(code, a, b);
}

void
nxu_chronicle_fail(nxu_u32 code, nxu_u64 a, nxu_u64 b)
{
	struct nxu_chronicle *c;

	c = nxu_chronicle_this();
	c->fail_code = code;
	c->fail_a = a;
	c->fail_b = b;
	c->fail_valid = 1U;
	nxu_chronicle_put(code, a, b);
	nxu_bbox_fail(code, a, b);
}
