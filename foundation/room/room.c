/*
 * Two rooms, one active at a time. EL0 cannot see the kernel or the other room.
 * A pulse copies one payload from the sender's own data page, or it is rejected.
 * One outstanding entry is owed. A second pulse refreshes the payload and does
 * not owe a second entry. Room 0 is the root.
 */

#include <nxu/room.h>
#include <nxu/log.h>
#include <nxu/fail.h>
#include <nxu/facts.h>
#include <nxu/image.h>
#include <nxu/range.h>
#include <nxu/time.h>
#include <nxu/cpu.h>
#include <silicon/aarch64/timer/timer.h>
#include <nxu/chronicle.h>
#include <silicon/aarch64/translate/mmu.h>
#include <silicon/aarch64/sysreg.h>
#include <silicon/aarch64/gicv3/gicv3.h>

#define ROOM_N 2U
#define EXIT_SVC     1U
#define EXIT_IRQ     2U
#define EXIT_FAULT   3U
#define EXIT_FOREIGN 4U
#define PAYLOAD      0x5055ULL
#define CANARY0      0xB0B0B0B0B0B0ULL
#define CANARY1      0xA11CE5A11CE5ULL

extern char nxu_blob_fault[];
extern char nxu_blob_fault_end[];
extern char nxu_blob_store[];
extern char nxu_blob_store_end[];
extern char nxu_blob_exec[];
extern char nxu_blob_exec_end[];
extern char nxu_blob_steal[];
extern char nxu_blob_steal_end[];
extern char nxu_blob_send[];
extern char nxu_blob_send_end[];
extern char nxu_blob_mal[];
extern char nxu_blob_mal_end[];
extern char nxu_blob_pay[];
extern char nxu_blob_pay_end[];
extern char nxu_blob_bad[];
extern char nxu_blob_bad_end[];
extern char nxu_blob_recv[];
extern char nxu_blob_recv_end[];
extern char nxu_blob_reply[];
extern char nxu_blob_reply_end[];
extern char nxu_blob_spin[];
extern char nxu_blob_spin_end[];
extern char nxu_vec_full[];

static nxu_u64 code_pa[ROOM_N];
static nxu_u64 data_pa[ROOM_N];
static nxu_u32 cur;
static nxu_u32 delivered;
static nxu_u32 rejected;
static nxu_u32 grant = ROOM_N;
static nxu_u64 armed_cval;

nxu_u32 nxu_room_reason;

extern void nxu_room_enter(nxu_u64 entry, nxu_u64 user_sp, nxu_u64 data,
    nxu_u64 fault_addr);

static nxu_u64
page_up(nxu_u64 n)
{
	return (n + NXU_PAGE_SIZE - 1ULL) & ~(NXU_PAGE_SIZE - 1ULL);
}

static void
tlbi_icache(void)
{
	nxu_mmu_tlbi();
	asm volatile("ic iallu; dsb sy; isb" ::: "memory");
}

static void
priv(nxu_u32 id)
{
	nxu_mmu_map(code_pa[id], NXU_PAGE_SIZE, NXU_MAP_RW);
	nxu_mmu_map(data_pa[id], NXU_PAGE_SIZE, NXU_MAP_RW);
}

static void
user_open(nxu_u32 id)
{
	nxu_u32 i;

	for (i = 0U; i < ROOM_N; i++)
		priv(i);
	nxu_mmu_map(code_pa[id], NXU_PAGE_SIZE, NXU_MAP_USER_RX);
	nxu_mmu_map(data_pa[id], NXU_PAGE_SIZE, NXU_MAP_USER_RW);
	tlbi_icache();
}

static void
clear_page(nxu_u64 pa)
{
	nxu_u64 *p;
	nxu_u32 i;

	p = (nxu_u64 *)(nxu_uptr)pa;
	for (i = 0U; i < NXU_PAGE_SIZE / 8U; i++)
		p[i] = 0U;
}

static void
copy_blob(nxu_u64 dst, const char *src, const char *end)
{
	nxu_u8 *d;
	nxu_u64 n;
	nxu_u64 i;

	n = (nxu_u64)(end - src);
	if (n == 0U || n > 64U)
		nxu_fail(NXU_FAIL_INVARIANT, n, 30U);
	d = (nxu_u8 *)(nxu_uptr)dst;
	for (i = 0U; i < n; i++)
		d[i] = (nxu_u8)src[i];
}

static int
inside(nxu_u32 id, nxu_u64 addr)
{
	if (addr < data_pa[id])
		return 0;
	if (addr >= data_pa[id] + NXU_PAGE_SIZE)
		return 0;
	return 1;
}

static void
seal(void)
{
	if (cur < ROOM_N)
		priv(cur);
	nxu_timer_ack();
	tlbi_icache();
}

static void
pulse(nxu_u64 arg0)
{
	nxu_u64 dest;
	nxu_u64 payload;
	nxu_u64 *slot;

	if (cur != 0U || !inside(0U, arg0)) {
		rejected++;
		NXU_LOG(NXU_EV_PULSE, NXU_SUB_CPU, arg0, 0U);
		return;
	}
	slot = (nxu_u64 *)(nxu_uptr)arg0;
	dest = slot[0];
	payload = slot[1];
	if (dest != 1U || payload != PAYLOAD) {
		rejected++;
		NXU_LOG(NXU_EV_PULSE, NXU_SUB_CPU, dest, 2U);
		return;
	}
	slot = (nxu_u64 *)(nxu_uptr)data_pa[1];
	slot[1] = payload;
	delivered++;
	grant = (nxu_u32)dest;
	NXU_LOG(NXU_EV_PULSE, NXU_SUB_CPU, dest, 1U);
}

static void
consume(nxu_u32 id)
{
	if (id == 0U)
		return;
	if (grant != id)
		nxu_fail(NXU_FAIL_INVARIANT, grant, id);
	grant = ROOM_N;
}

static void
refuse(nxu_u32 id)
{
	if (id == 0U || grant == id)
		nxu_fail(NXU_FAIL_INVARIANT, grant, 36U);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, id, 5U);
}

nxu_u32
nxu_room_exception(nxu_u64 esr, nxu_u64 elr, nxu_u64 arg0)
{
	nxu_u32 ec;
	nxu_u32 imm;

	(void)elr;
	ec = (nxu_u32)((esr >> 26) & 0x3fULL);
	if (ec == 0x15U) {
		imm = (nxu_u32)(esr & 0xffffULL);
		if (imm == 1U) {
			pulse(arg0);
			return 0U;
		}
		if (imm == 0U) {
			nxu_room_reason = EXIT_SVC;
			seal();
			return 1U;
		}
	}
	nxu_room_reason = EXIT_FAULT;
	seal();
	return 1U;
}

void
nxu_room_irq(void)
{
	nxu_u32 intid;

	intid = nxu_gic_iar();
	if (intid >= 1020U) {
		nxu_room_reason = EXIT_FAULT;
		seal();
		return;
	}
	/* The deadline is the kernel's timer. Any other interrupt ends the
	 * room and returns here. It is not delivered as a Pulse.
	 */
	if (intid != nxu_facts.timer_ppi) {
		nxu_room_reason = EXIT_FOREIGN;
		seal();
		nxu_gic_eoi(intid);
		return;
	}
	nxu_room_reason = EXIT_IRQ;
	seal();
	nxu_gic_eoi(intid);
}

static void
run(nxu_u32 id, const char *src, const char *end, nxu_u64 fault,
    int wipe, int poke)
{
	nxu_u64 ticks;
	nxu_u64 now;

	consume(id);
	if (wipe)
		clear_page(data_pa[id]);
	priv(id);
	copy_blob(code_pa[id], src, end);
	cur = id;
	user_open(id);
	nxu_daif_mask();
	/* The accepted counter frequency, not a fresh CNTFRQ read. A zero
	 * read programs a compare of now, so ISTATUS never clears.
	 */
	ticks = nxu_facts.timer_freq / 10ULL;
	if (ticks == 0U || ticks > 0x7fffffffULL)
		nxu_fail(NXU_FAIL_TIMER, ticks, 8U);
	now = nxu_cntpct();
	nxu_msr(cntp_cval_el0, now + ticks);
	nxu_isb();
	nxu_msr(cntp_tval_el0, ticks);
	nxu_isb();
	nxu_msr(cntp_ctl_el0, 1ULL);
	nxu_isb();
	{
		nxu_u32 me;

		me = nxu_this_logical();
		if (me >= NXU_CPU_BOUND)
			me = 0U;
		if (nxu_cpu[me].gicr != 0U)
			nxu_gicr_clear_pending(nxu_cpu[me].gicr, nxu_facts.timer_ppi);
		if (poke)
			nxu_gic_sgi(NXU_SGI_IPI, nxu_cpu[me].mpidr);
	}
	if (nxu_timer_cval_el1() <= nxu_cntpct() || nxu_timer_istatus() != 0U) {
		NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_T0, nxu_cntfrq());
		NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_TVAL, ticks);
		NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_CNTP_CVAL,
		    nxu_timer_cval_el1());
		NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_TNOW, nxu_cntpct());
		NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_CNTP_CTL,
		    nxu_timer_ctl_el1());
		nxu_fail(NXU_FAIL_TIMER, nxu_timer_ctl_el1(), 9U);
	}
	armed_cval = nxu_timer_cval_el1();
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, id, 6U);
	nxu_room_enter(code_pa[id], data_pa[id] + 0xf00ULL, data_pa[id], fault);
	nxu_dsb();
}

static void
expect(nxu_u32 reason)
{
	if (nxu_room_reason != reason)
		nxu_fail(NXU_FAIL_INVARIANT, nxu_room_reason, reason);
}

static int
el0_visible(nxu_u64 leaf)
{
	return (leaf & (1ULL << 6)) != 0U;
}

static void
check_time(int hit)
{
	nxu_u64 now;

	now = nxu_cntpct();
	if (hit) {
		if (now < armed_cval)
			nxu_fail(NXU_FAIL_TIMER, now, armed_cval);
		return;
	}
	if (now >= armed_cval)
		nxu_fail(NXU_FAIL_TIMER, now, armed_cval);
}

static nxu_u64 *
slot(nxu_u32 id)
{
	return (nxu_u64 *)(nxu_uptr)data_pa[id];
}

static void
plant(nxu_u32 id, nxu_u64 val)
{
	slot(id)[4] = val;
}

static void
same(nxu_u32 id, nxu_u64 val)
{
	if (slot(id)[4] != val)
		nxu_fail(NXU_FAIL_INVARIANT, slot(id)[4], val);
}

static void
check_boundary(void)
{
	nxu_u64 kernel;
	nxu_u64 code0;
	nxu_u64 data0;
	nxu_u64 code1;
	nxu_u64 data1;

	user_open(0U);
	kernel = nxu_mmu_leaf(nxu_image.base);
	code0 = nxu_mmu_leaf(code_pa[0]);
	data0 = nxu_mmu_leaf(data_pa[0]);
	code1 = nxu_mmu_leaf(code_pa[1]);
	data1 = nxu_mmu_leaf(data_pa[1]);
	priv(0U);
	nxu_mmu_tlbi();
	if ((kernel & (1ULL << 54)) == 0U || el0_visible(kernel))
		nxu_fail(NXU_FAIL_INVARIANT, kernel, 31U);
	if (!el0_visible(code0) || (code0 & (1ULL << 54)) != 0U)
		nxu_fail(NXU_FAIL_INVARIANT, code0, 32U);
	if (!el0_visible(data0) || (data0 & (1ULL << 54)) == 0U)
		nxu_fail(NXU_FAIL_INVARIANT, data0, 33U);
	if (el0_visible(code1) || el0_visible(data1))
		nxu_fail(NXU_FAIL_INVARIANT, code1, 40U);
	NXU_LOG(NXU_EV_BOUND, NXU_SUB_SPACE, 0U, 0U);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, 0U, 1U);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, 1U, 1U);
}

static void
contained(nxu_u32 id)
{
	expect(EXIT_FAULT);
	check_time(0);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, id, 4U);
}

static void
early_svc(void)
{
	expect(EXIT_SVC);
	check_time(0);
}

static void
send_once(void)
{
	nxu_u32 d;

	d = delivered;
	run(0U, nxu_blob_send, nxu_blob_send_end, 0U, 0, 0);
	early_svc();
	if (delivered != d + 1U)
		nxu_fail(NXU_FAIL_INVARIANT, delivered, d);
}

void
nxu_rooms_run(void)
{
	nxu_u64 base;
	nxu_u64 need;
	nxu_u64 kernel;
	nxu_u32 rej;

	base = page_up(nxu_image.base + nxu_image.size);
	need = NXU_PAGE_SIZE * 4U;
	if (!nxu_range_contains(nxu_facts.mem[0].range.base,
	    nxu_facts.mem[0].range.size, base, need))
		nxu_fail(NXU_FAIL_NO_RAM, base, need);
	code_pa[0] = base;
	data_pa[0] = base + NXU_PAGE_SIZE;
	code_pa[1] = base + 2U * NXU_PAGE_SIZE;
	data_pa[1] = base + 3U * NXU_PAGE_SIZE;
	priv(0U);
	priv(1U);
	clear_page(data_pa[0]);
	clear_page(data_pa[1]);
	plant(0U, CANARY0);
	plant(1U, CANARY1);
	/* EL0 must not read or write the physical timer. A room that could
	 * move the compare could spend another room's slice.
	 */
	nxu_msr(cntkctl_el1, 0ULL);
	nxu_isb();
	if ((nxu_mrs(cntkctl_el1) & ((1ULL << 9) | 1ULL)) != 0U)
		nxu_fail(NXU_FAIL_TIMER, nxu_mrs(cntkctl_el1), 11U);
	check_boundary();
	grant = ROOM_N;
	kernel = (nxu_u64)(nxu_uptr)nxu_vec_full;

	run(0U, nxu_blob_fault, nxu_blob_fault_end, kernel, 0, 0);
	contained(0U);
	same(1U, CANARY1);

	run(0U, nxu_blob_fault, nxu_blob_fault_end, data_pa[1] + 32U, 0, 0);
	contained(0U);
	same(1U, CANARY1);

	run(0U, nxu_blob_store, nxu_blob_store_end, data_pa[1] + 32U, 0, 0);
	contained(0U);
	same(1U, CANARY1);

	run(0U, nxu_blob_exec, nxu_blob_exec_end, data_pa[0], 0, 0);
	contained(0U);

	run(0U, nxu_blob_exec, nxu_blob_exec_end, code_pa[1], 0, 0);
	contained(0U);
	same(1U, CANARY1);

	run(0U, nxu_blob_store, nxu_blob_store_end, code_pa[0], 0, 0);
	contained(0U);
	if (*(nxu_u32 *)(nxu_uptr)code_pa[0] != *(nxu_u32 *)(nxu_uptr)nxu_blob_store)
		nxu_fail(NXU_FAIL_INVARIANT, *(nxu_u32 *)(nxu_uptr)code_pa[0], 42U);

	if (nxu_facts.uart_valid) {
		run(0U, nxu_blob_fault, nxu_blob_fault_end,
		    nxu_facts.uart.range.base, 0, 0);
		contained(0U);
	}

	refuse(1U);
	rej = rejected;
	run(0U, nxu_blob_mal, nxu_blob_mal_end, 0U, 0, 0);
	early_svc();
	run(0U, nxu_blob_pay, nxu_blob_pay_end, 0U, 0, 0);
	early_svc();
	run(0U, nxu_blob_bad, nxu_blob_bad_end, kernel, 0, 0);
	early_svc();
	if (rejected < rej + 3U)
		nxu_fail(NXU_FAIL_INVARIANT, rejected, rej);
	if (slot(1)[1] != 0U)
		nxu_fail(NXU_FAIL_INVARIANT, slot(1)[1], 41U);
	refuse(1U);
	same(1U, CANARY1);

	send_once();
	send_once();
	if (slot(1)[1] != PAYLOAD || slot(1)[3] != 0U)
		nxu_fail(NXU_FAIL_INVARIANT, slot(1)[3], PAYLOAD);
	same(1U, CANARY1);
	run(1U, nxu_blob_recv, nxu_blob_recv_end, 0U, 0, 0);
	early_svc();
	if (slot(1)[2] != PAYLOAD)
		nxu_fail(NXU_FAIL_INVARIANT, slot(1)[2], PAYLOAD);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, 1U, 2U);
	refuse(1U);
	same(0U, CANARY0);

	send_once();
	run(1U, nxu_blob_store, nxu_blob_store_end, data_pa[0] + 32U, 0, 0);
	contained(1U);
	same(0U, CANARY0);
	same(1U, CANARY1);

	send_once();
	run(1U, nxu_blob_fault, nxu_blob_fault_end, kernel, 0, 0);
	contained(1U);

	send_once();
	run(1U, nxu_blob_exec, nxu_blob_exec_end, code_pa[0], 0, 0);
	contained(1U);

	send_once();
	run(1U, nxu_blob_store, nxu_blob_store_end, code_pa[1], 0, 0);
	contained(1U);
	if (*(nxu_u32 *)(nxu_uptr)code_pa[1] != *(nxu_u32 *)(nxu_uptr)nxu_blob_store)
		nxu_fail(NXU_FAIL_INVARIANT, *(nxu_u32 *)(nxu_uptr)code_pa[1], 43U);

	send_once();
	run(1U, nxu_blob_steal, nxu_blob_steal_end, 0U, 0, 0);
	contained(1U);
	if (nxu_timer_cval_el1() != armed_cval)
		nxu_fail(NXU_FAIL_TIMER, nxu_timer_cval_el1(), armed_cval);

	rej = rejected;
	send_once();
	run(1U, nxu_blob_reply, nxu_blob_reply_end, 0U, 0, 0);
	early_svc();
	if (rejected != rej + 1U)
		nxu_fail(NXU_FAIL_INVARIANT, rejected, rej);
	refuse(1U);

	send_once();
	run(1U, nxu_blob_spin, nxu_blob_spin_end, 0U, 0, 1);
	expect(EXIT_FOREIGN);
	check_time(0);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, 1U, 7U);
	same(0U, CANARY0);
	same(1U, CANARY1);

	send_once();
	run(1U, nxu_blob_spin, nxu_blob_spin_end, 0U, 0, 0);
	expect(EXIT_IRQ);
	check_time(1);
	NXU_LOG(NXU_EV_ROOM, NXU_SUB_CPU, 1U, 3U);
	same(0U, CANARY0);

	NXU_LOG(NXU_EV_APP, NXU_SUB_BOOT, 0U, 0U);
}
