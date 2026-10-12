/*
 * NXU MAP
 * Layer: foundation observe
 * Owns: text form of a log record
 * Consumes: record; sink
 * Produces: one line, plus the boot identity on the boot event
 * Hardware: none
 * Forbidden: policy; retry that affects the caller
 * Invariant: presentation failure is silence
 */

#include <nxu/console.h>
#include <nxu/log.h>
#include <nxu/sink.h>
#include <nxu/cpu.h>
#include <nxu/config.h>

static int
console_ok(void)
{
	if (!nxu_sink_ready)
		return 0;
	return 1;
}

#ifdef NXU_DEBUG
static void
field(const char *name, nxu_u64 v)
{
	nxu_sink_write(" ");
	nxu_sink_write(name);
	nxu_sink_write("=0x");
	nxu_sink_hex(v, 16U);
}
#endif

static void
sink_u32(nxu_u32 v)
{
	char buf[11];
	char *p;

	p = buf + 10;
	*p = 0;
	if (v == 0U) {
		nxu_sink_write("0");
		return;
	}
	while (v != 0U) {
		*--p = (char)('0' + (v % 10U));
		v /= 10U;
	}
	nxu_sink_write(p);
}

static const char *
base_name(const char *path)
{
	const char *s;

	if (path == NXU_NULL)
		return "-";
	s = path;
	while (*path != 0) {
		if (*path == '/')
			s = path + 1;
		path++;
	}
	return s;
}

/*
 * Names are written from each branch. A pointer table would hold link-time
 * addresses, and QEMU loads the image at a different physical address.
 */
static void
write_mach(nxu_u64 id)
{
	if (id == NXU_MACH_WAKER)
		nxu_sink_write("waker");
	else if (id == NXU_MACH_GICD_CTLR)
		nxu_sink_write("gicd-ctlr");
	else if (id == NXU_MACH_IGROUPR0)
		nxu_sink_write("igroupr0");
	else if (id == NXU_MACH_ISENABLER0)
		nxu_sink_write("isenabler0");
	else if (id == NXU_MACH_ISPENDR0)
		nxu_sink_write("ispendr0");
	else if (id == NXU_MACH_IGRPMODR0)
		nxu_sink_write("igrpmodr0");
	else if (id == NXU_MACH_DAIF)
		nxu_sink_write("daif");
	else if (id == NXU_MACH_HCR)
		nxu_sink_write("hcr");
	else if (id == NXU_MACH_ISR)
		nxu_sink_write("isr");
	else if (id == NXU_MACH_IGRPEN1)
		nxu_sink_write("igrpen1");
	else if (id == NXU_MACH_HPPIR1)
		nxu_sink_write("hppir1");
	else if (id == NXU_MACH_HPPIR0)
		nxu_sink_write("hppir0");
	else if (id == NXU_MACH_T0)
		nxu_sink_write("t0");
	else if (id == NXU_MACH_TNOW)
		nxu_sink_write("tnow");
	else if (id == NXU_MACH_TVAL)
		nxu_sink_write("tval");
	else if (id == NXU_MACH_ISTATUS)
		nxu_sink_write("istatus");
	else if (id == NXU_MACH_CNTP_CTL)
		nxu_sink_write("cntp-ctl");
	else if (id == NXU_MACH_CNTP_CVAL)
		nxu_sink_write("cntp-cval");
	else
		nxu_sink_write("mach");
}

static void
say(const char *sentence)
{
	nxu_sink_write("[NXU] ");
	nxu_sink_write(sentence);
	nxu_sink_nl();
}

static void
write_aff64(nxu_u64 m)
{
	nxu_sink_write("Aff3.");
	sink_u32((nxu_u32)((m >> 32) & 0xffULL));
	nxu_sink_write(" Aff2.");
	sink_u32((nxu_u32)((m >> 16) & 0xffULL));
	nxu_sink_write(" Aff1.");
	sink_u32((nxu_u32)((m >> 8) & 0xffULL));
	nxu_sink_write(" Aff0.");
	sink_u32((nxu_u32)(m & 0xffULL));
}

static void
write_aff32(nxu_u64 a)
{
	nxu_sink_write("Aff3.");
	sink_u32((nxu_u32)((a >> 24) & 0xffULL));
	nxu_sink_write(" Aff2.");
	sink_u32((nxu_u32)((a >> 16) & 0xffULL));
	nxu_sink_write(" Aff1.");
	sink_u32((nxu_u32)((a >> 8) & 0xffULL));
	nxu_sink_write(" Aff0.");
	sink_u32((nxu_u32)(a & 0xffULL));
}

void
nxu_console_record(const struct nxu_log_record *r)
{
	if (r == NXU_NULL || !console_ok())
		return;

	if (r->event == NXU_EV_BOOT) {
		nxu_sink_write("a\n");
		nxu_sink_write("nxu\n");
		nxu_sink_write("Not eXactly Unix\n");
		nxu_sink_write("THE ZERO ONE\n");
		say("Entered");
		return;
	}

	switch (r->event) {
	case NXU_EV_DTB:
		say(r->v1 != 0U ? "DTB validated" : "DTB absent");
		break;
	case NXU_EV_MEMORY:
		say("RAM detected");
		break;
	case NXU_EV_MMU:
		say(r->v1 == 1U ? "EL1 translation established" : "MMU established");
		break;
	case NXU_EV_EXCEPTION:
		say("EL1 exception vectors established");
		break;
	case NXU_EV_CPU:
		if (r->v2 == NXU_CPU_DETECT) {
			say("Detecting processor topology.");
			break;
		}
		if (r->v2 == NXU_CPU_CLUSTER) {
			nxu_sink_write("[NXU] Detected cluster ");
			sink_u32((nxu_u32)r->v0);
			nxu_sink_write(" with ");
			sink_u32((nxu_u32)r->v1);
			nxu_sink_write(" processors.");
			nxu_sink_nl();
			break;
		}
		if (r->v2 == NXU_CPU_RDIST) {
			nxu_sink_write("[NXU] A redistributor reports affinity ");
			write_aff32(r->v0);
			nxu_sink_write(".");
			nxu_sink_nl();
			break;
		}
		nxu_sink_write("[NXU] CPU ");
		sink_u32((nxu_u32)r->v0);
		if (r->v2 == NXU_CPU_PRESENT) {
			nxu_sink_write(" is present with MPIDR affinity ");
			write_aff64(r->v1);
			nxu_sink_write(".");
		} else if (r->v2 == NXU_CPU_INIT) {
			nxu_sink_write(" accepted for initialization.");
		} else if (r->v2 == NXU_CPU_ENTER) {
			nxu_sink_write(" entered the NXU kernel.");
		} else if (r->v2 == NXU_CPU_EL1) {
			nxu_sink_write(" established its EL1 execution state.");
		} else if (r->v2 == NXU_CPU_VBAR) {
			nxu_sink_write(" established its exception vectors.");
		} else if (r->v2 == NXU_CPU_GIC) {
			nxu_sink_write(" established its GIC redistributor association.");
		} else {
			nxu_sink_write(" reported a processor event.");
		}
		nxu_sink_nl();
		break;
	case NXU_EV_GIC:
		say("GICv3 detected");
		break;
	case NXU_EV_GIC_INIT:
		say("GICv3 established");
		break;
	case NXU_EV_TIMER:
		say("CNTP established");
		nxu_sink_write("  Interrupt source PPI ");
		sink_u32((nxu_u32)r->v0);
		nxu_sink_nl();
		break;
	case NXU_EV_TIMER_FIRE:
		say("CNTP deadline observed");
		break;
	case NXU_EV_CPU_ONLINE:
		nxu_sink_write("[NXU] CPU ");
		sink_u32((nxu_u32)r->v1);
		nxu_sink_write(" is now online.");
		nxu_sink_nl();
		break;
	case NXU_EV_CPU_SET:
		nxu_sink_write("[NXU] SMP execution established across ");
		sink_u32((nxu_u32)r->v0);
		nxu_sink_write(" processors.");
		nxu_sink_nl();
		break;
	case NXU_EV_IPI:
		if (r->v2 == NXU_IPI_RECV) {
			nxu_sink_write("[NXU] CPU ");
			sink_u32((nxu_u32)r->v1);
			nxu_sink_write(" received the IPI from CPU ");
			sink_u32((nxu_u32)r->v0);
			nxu_sink_write(".");
		} else {
			nxu_sink_write("[NXU] Sending an IPI from CPU ");
			sink_u32((nxu_u32)r->v0);
			nxu_sink_write(" to CPU ");
			sink_u32((nxu_u32)r->v1);
			nxu_sink_write(".");
		}
		nxu_sink_nl();
		break;
	case NXU_EV_KERNEL:
		say("NXU kernel execution is established.");
		break;
	case NXU_EV_BOUND:
		say("The spatial boundary is established.");
		break;
	case NXU_EV_ROOM:
		if (r->v1 == 6U) {
			nxu_sink_write("[NXU] Room ");
			sink_u32((nxu_u32)r->v0);
			nxu_sink_write(" entered its execution region.");
			nxu_sink_nl();
			break;
		}
		nxu_sink_write("[NXU] Room ");
		sink_u32((nxu_u32)r->v0);
		if (r->v1 == 1U)
			nxu_sink_write(" now owns its execution region.");
		else if (r->v1 == 2U)
			nxu_sink_write(" returned by its own request.");
		else if (r->v1 == 3U)
			nxu_sink_write(" reached its temporal deadline, and the kernel reclaimed execution.");
		else if (r->v1 == 5U)
			nxu_sink_write(" was refused entry.");
		else if (r->v1 == 7U)
			nxu_sink_write(" was presented with an interrupt it does not own, and the kernel reclaimed execution.");
		else
			nxu_sink_write(" had an access outside its region contained.");
		nxu_sink_nl();
		break;
	case NXU_EV_PULSE:
		if (r->v1 == 1U)
			say("Room 0 named Room 1 through an authorized Pulse.");
		else if (r->v1 == 2U)
			say("The Pulse was rejected. Its destination or payload is not permitted.");
		else
			say("The Pulse was rejected.");
		break;
	case NXU_EV_APP:
		say("The first NXU application has completed.");
		break;
	case NXU_EV_FAULT:
		say("Exception fault");
		nxu_sink_write("  ESR ");
		nxu_sink_hex(r->v0, 16U);
		nxu_sink_write(" FAR ");
		nxu_sink_hex(r->v1, 16U);
		nxu_sink_write(" ELR ");
		nxu_sink_hex(r->v2, 16U);
		nxu_sink_nl();
		break;
	case NXU_EV_MACHINE:
		nxu_sink_write("  ");
		write_mach(r->v0);
		nxu_sink_write(" ");
		nxu_sink_hex(r->v1, 16U);
		nxu_sink_nl();
		break;
	case NXU_EV_FAIL:
		say("Halted");
		nxu_sink_write("  ");
		nxu_sink_write(base_name(r->file));
		nxu_sink_write(" ");
		nxu_sink_write(r->func != NXU_NULL ? r->func : "-");
		nxu_sink_write(" ");
		sink_u32(r->line);
		nxu_sink_write(" code ");
		nxu_sink_hex(r->v0, 8U);
		nxu_sink_nl();
		break;
	default:
		say("Event");
		break;
	}
}

void
nxu_debug(const char *tag, const char *msg)
{
#ifndef NXU_DEBUG
	(void)tag;
	(void)msg;
	return;
#else
	if (!console_ok())
		return;
	nxu_sink_write("[nxu] note debug ");
	nxu_sink_write(tag);
	nxu_sink_write(" ");
	nxu_sink_write(msg);
	nxu_sink_nl();
#endif
}

void
nxu_debug_u64(const char *key, nxu_u64 v)
{
#ifndef NXU_DEBUG
	(void)key;
	(void)v;
	return;
#else
	if (!console_ok())
		return;
	nxu_sink_write("[nxu] note debug");
	field(key, v);
	nxu_sink_nl();
#endif
}
