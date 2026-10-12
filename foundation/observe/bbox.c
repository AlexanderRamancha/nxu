/*
 * NXU MAP
 *
 *                    FOUNDATION OBSERVE
 *                           │
 *                 ┌─────────┴─────────┐
 *                 ▼                   ▼
 *             CHRONICLE            BLACK BOX
 *                                     │
 *                                     ▼
 *                              PRESERVED RING
 *                                     │
 *                                     ▼
 *                                 FAIL DUMP
 *
 * Layer: foundation observe
 * Owns: per-CPU bounded ring; immortal last-failure
 * Consumes: code, a, b from observe path
 * Produces: dumpable evidence at fail
 * Hardware: none
 * Forbidden: allocation; pointers retained; policy decisions
 * Invariant: overflow overwrites oldest non-failure; fail slot stays
 */

#include <nxu/bbox.h>
#include <nxu/config.h>
#include <nxu/cpu.h>
#include <nxu/sink.h>

struct nxu_bbox_rec {
	nxu_u32 code;
	nxu_u32 cpu;
	nxu_u64 a;
	nxu_u64 b;
};

struct nxu_bbox {
	nxu_u32 next;
	nxu_u32 count;
	nxu_u32 fail_code;
	nxu_u8  fail_valid;
	nxu_u64 fail_a;
	nxu_u64 fail_b;
	struct nxu_bbox_rec rec[NXU_BBOX_CAP];
};

static struct nxu_bbox nxu_bbox_cpu[NXU_CPU_BOUND];

void
nxu_bbox_init(void)
{
	nxu_u32 cpu;
	nxu_u32 i;

	for (cpu = 0U; cpu < NXU_CPU_BOUND; cpu++) {
		nxu_bbox_cpu[cpu].next = 0U;
		nxu_bbox_cpu[cpu].count = 0U;
		nxu_bbox_cpu[cpu].fail_code = 0U;
		nxu_bbox_cpu[cpu].fail_valid = 0U;
		nxu_bbox_cpu[cpu].fail_a = 0U;
		nxu_bbox_cpu[cpu].fail_b = 0U;
		for (i = 0U; i < NXU_BBOX_CAP; i++) {
			nxu_bbox_cpu[cpu].rec[i].code = 0U;
			nxu_bbox_cpu[cpu].rec[i].cpu = cpu;
			nxu_bbox_cpu[cpu].rec[i].a = 0U;
			nxu_bbox_cpu[cpu].rec[i].b = 0U;
		}
	}
}

static struct nxu_bbox *
bbox_this(void)
{
	nxu_u32 id;

	id = nxu_this_logical();
	if (id >= NXU_CPU_BOUND)
		id = 0U;
	return &nxu_bbox_cpu[id];
}

void
nxu_bbox_put(nxu_u32 code, nxu_u64 va, nxu_u64 vb)
{
	struct nxu_bbox *box;
	struct nxu_bbox_rec *r;
	nxu_u32 i;
	nxu_u32 id;

	box = bbox_this();
	id = nxu_this_logical();
	if (id >= NXU_CPU_BOUND)
		id = 0U;
	i = box->next % NXU_BBOX_CAP;
	r = &box->rec[i];
	r->code = code;
	r->cpu = id;
	r->a = va;
	r->b = vb;
	box->next = i + 1U;
	box->count++;
}

void
nxu_bbox_fail(nxu_u32 code, nxu_u64 va, nxu_u64 vb)
{
	struct nxu_bbox *box;

	nxu_bbox_put(code, va, vb);
	box = bbox_this();
	box->fail_code = code;
	box->fail_a = va;
	box->fail_b = vb;
	box->fail_valid = 1U;
}

void
nxu_bbox_dump(void)
{
	nxu_u32 cpu;
	nxu_u32 n;
	nxu_u32 i;
	nxu_u32 idx;
	struct nxu_bbox *box;

	if (!nxu_sink_ready)
		return;

	for (cpu = 0U; cpu < NXU_CPU_BOUND; cpu++) {
		box = &nxu_bbox_cpu[cpu];
		if (box->count == 0U && box->fail_valid == 0U)
			continue;
		nxu_sink_write("[NXU] Black box");
		nxu_sink_write(" cpu=0x");
		nxu_sink_hex(cpu, 16U);
		nxu_sink_nl();
		n = box->count;
		if (n > NXU_BBOX_CAP)
			n = NXU_BBOX_CAP;
		for (i = 0U; i < n; i++) {
			if (box->count > NXU_BBOX_CAP)
				idx = (box->next + i) % NXU_BBOX_CAP;
			else
				idx = i;
			nxu_sink_write("    rec ");
			nxu_sink_hex(box->rec[idx].code, 8U);
			nxu_sink_write(" ");
			nxu_sink_hex(box->rec[idx].a, 16U);
			nxu_sink_write(" ");
			nxu_sink_hex(box->rec[idx].b, 16U);
			nxu_sink_nl();
		}
		if (box->fail_valid) {
			nxu_sink_write("[NXU] Black box last failure");
			nxu_sink_write(" code=0x");
			nxu_sink_hex(box->fail_code, 8U);
			nxu_sink_write(" a=0x");
			nxu_sink_hex(box->fail_a, 16U);
			nxu_sink_write(" b=0x");
			nxu_sink_hex(box->fail_b, 16U);
			nxu_sink_nl();
		}
	}
}
