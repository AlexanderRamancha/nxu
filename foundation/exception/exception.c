/*
 * NXU MAP
 * Layer: foundation exception
 * Owns: S7 VBAR; C path from silicon vectors; save into bound context
 * Consumes: nxu_cpu_exec_context, GIC IAR/EOI, ESR/FAR via silicon
 * Produces: faulted context + halt on unexpected class; IRQ dispatch by INTID
 * Hardware: VBAR, ESR, FAR, ICC_IAR/EOI (ops only)
 * Forbidden: function-pointer handlers; continue after unexpected class
 * Invariant: VBAR is NXU's before unmask; ESR/FAR recorded before halt
 */

#include <nxu/exception.h>
#include <nxu/context.h>
#include <nxu/cpu.h>
#include <nxu/intake.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/signal.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <silicon/aarch64/sysreg.h>
#include <silicon/aarch64/gicv3/gicv3.h>

void
nxu_exception(nxu_u32 kind)
{
	nxu_u64 esr;
	nxu_u64 far;
	nxu_u64 elr;
	nxu_u32 el;
	nxu_u32 id;
	nxu_cpu_exec_context *ctx;

	el = nxu_current_el();
	if (el == NXU_EL3) {
		esr = nxu_mrs(S3_6_C5_C2_0);
		far = nxu_mrs(S3_6_C6_C0_0);
		elr = nxu_mrs(S3_6_C4_C0_1);
	} else if (el == NXU_EL2) {
		esr = nxu_mrs_s3_4_c5_c2_0();
		far = nxu_mrs_s3_4_c6_c0_0();
		elr = nxu_mrs(S3_4_C4_C0_1);
	} else {
		esr = nxu_mrs(esr_el1);
		far = nxu_mrs(far_el1);
		elr = nxu_mrs(elr_el1);
	}

	id = nxu_this_logical();
	if (id >= NXU_CPU_BOUND)
		id = 0U;
	ctx = &nxu_cpu[id].ctx;
	ctx->esr = esr;
	ctx->far = far;
	ctx->el = el;
	ctx->life = NXU_CTX_FAULTED;
	ctx->intid = 0U;
	nxu_chronicle_put(NXU_REC_S7_FAULT, esr, far);
	NXU_LOG3(NXU_EV_FAULT, NXU_SUB_EXC, esr, far, elr);

	if (kind == 4U)
		nxu_fail(NXU_FAIL_SERROR, esr, far);
	nxu_fail(NXU_FAIL_SYNC, esr, far);
}

void
nxu_irq_c(void)
{
	nxu_u32 intid;
	nxu_u32 id;
	nxu_cpu_exec_context *ctx;

	intid = nxu_gic_iar();
	id = nxu_this_logical();
	if (id >= NXU_CPU_BOUND)
		id = 0U;
	ctx = &nxu_cpu[id].ctx;
	ctx->intid = intid;
	ctx->daif = 0U;

	if (intid >= 1020U && intid <= 1023U) {
		nxu_gic_eoi(intid);
		ctx->intid = 0U;
		return;
	}
	nxu_irq_dispatch(intid);
	nxu_gic_eoi(intid);
	ctx->intid = 0U;
}

void
nxu_s7_exception(void)
{
	nxu_u32 el;
	extern char nxu_vec_full[];
	nxu_u64 vbar;

	nxu_context_bind_boot();

	el = nxu_control_el();
	if (el != NXU_EL1)
		nxu_fail(NXU_FAIL_BAD_EL, el, 3U);
	vbar = (nxu_u64)(nxu_uptr)nxu_vec_full;
	nxu_msr(vbar_el1, vbar);
	nxu_isb();
	nxu_chronicle_put(NXU_REC_S7_CTX, 0U, nxu_cpu[0].ctx.life);
	nxu_chronicle_put(NXU_REC_S7_VBAR, vbar, el);
	NXU_LOG(NXU_EV_EXCEPTION, NXU_SUB_EXC, vbar, el);
	nxu_debug("EXC", "VBAR programmed");
	nxu_debug_u64("vbar", vbar);
	nxu_debug_u64("el", el);
}
