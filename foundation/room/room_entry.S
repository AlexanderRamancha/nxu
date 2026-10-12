.section .text
.align 2

.global nxu_blob_fault
nxu_blob_fault:
	ldr	x2, [x1]
	svc	#0
.global nxu_blob_fault_end
nxu_blob_fault_end:

.global nxu_blob_store
nxu_blob_store:
	str	xzr, [x1]
	svc	#0
.global nxu_blob_store_end
nxu_blob_store_end:

.global nxu_blob_exec
nxu_blob_exec:
	br	x1
.global nxu_blob_exec_end
nxu_blob_exec_end:

.global nxu_blob_steal
nxu_blob_steal:
	msr	cntp_tval_el0, xzr
	svc	#0
.global nxu_blob_steal_end
nxu_blob_steal_end:

.global nxu_blob_send
nxu_blob_send:
	mov	x2, #1
	str	x2, [x0]
	mov	x2, #0x5055
	str	x2, [x0, #8]
	mov	x2, #0x1111
	str	x2, [x0, #16]
	str	x2, [x0, #24]
	svc	#1
	svc	#0
.global nxu_blob_send_end
nxu_blob_send_end:

.global nxu_blob_mal
nxu_blob_mal:
	mov	x2, #2
	str	x2, [x0]
	mov	x2, #0x5055
	str	x2, [x0, #8]
	svc	#1
	svc	#0
.global nxu_blob_mal_end
nxu_blob_mal_end:

.global nxu_blob_pay
nxu_blob_pay:
	mov	x2, #1
	str	x2, [x0]
	mov	x2, #1
	str	x2, [x0, #8]
	svc	#1
	svc	#0
.global nxu_blob_pay_end
nxu_blob_pay_end:

.global nxu_blob_bad
nxu_blob_bad:
	mov	x0, x1
	svc	#1
	svc	#0
.global nxu_blob_bad_end
nxu_blob_bad_end:

.global nxu_blob_reply
nxu_blob_reply:
	mov	x2, #0
	str	x2, [x0]
	mov	x2, #0x5055
	str	x2, [x0, #8]
	svc	#1
	svc	#0
.global nxu_blob_reply_end
nxu_blob_reply_end:

.global nxu_blob_recv
nxu_blob_recv:
	ldr	x2, [x0, #8]
	str	x2, [x0, #16]
	svc	#0
.global nxu_blob_recv_end
nxu_blob_recv_end:

.global nxu_blob_spin
nxu_blob_spin:
	b	nxu_blob_spin
.global nxu_blob_spin_end
nxu_blob_spin_end:

.global nxu_room_enter
nxu_room_enter:
	stp	x29, x30, [sp, #-16]!
	msr	sp_el0, x1
	msr	elr_el1, x0
	mov	x4, #0x340
	msr	spsr_el1, x4
	mov	x0, x2
	mov	x1, x3
	eret
.global nxu_room_return
nxu_room_return:
	ldp	x29, x30, [sp], #16
	ret
