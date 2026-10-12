#!/bin/sh
# Host-side architectural checks. Probe, not TCB.
set -e
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

fail() {
	echo "check FAIL: $*" >&2
	exit 1
}

if grep -n 'research/' Makefile | grep -vE '^[0-9]+:[[:space:]]*#' | grep -q .; then
	fail "Makefile TCB path includes research/"
fi

if grep -REn --include='*.c' --include='*.h' --include='*.S' --include='*.ld' \
    'research/' foundation silicon image include 2>/dev/null | grep -q .; then
	fail "research/ reachable from TCB sources"
fi

forbidden='0x40000000|0x40080000|0x08000000|0x080A0000|0x09000000|0x2F000000|0x2F100000|0x1C090000'
hits=$(grep -REn --include='*.c' --include='*.h' --include='*.S' --include='*.ld' \
    "$forbidden" foundation silicon image include Makefile 2>/dev/null || true)
if [ -n "$hits" ]; then
	echo "$hits" >&2
	fail "board addresses in architectural files"
fi

if grep -REn --include='*.c' --include='*.h' --include='*.S' \
    'Wall|Edge|Fabric' foundation silicon include image 2>/dev/null | grep -q .; then
	fail "research names in TCB"
fi

if grep -REn --include='*.c' --include='*.h' \
    'request_irq|handler_fn|nxu_irq_fn|nxu_sched_|nxu_domain_|current_task|runqueue' \
    foundation silicon include image 2>/dev/null | grep -q .; then
	fail "historical OS architecture in TCB"
fi

if grep -REn --include='*.c' --include='*.h' --include='*.S' \
    'cpu_start\(1\)|secondary_stack_cpu1' foundation silicon include image 2>/dev/null | grep -q .; then
	fail "historical CPU1 symbols"
fi

if [ -d kernel ] || [ -d arch ] || [ -d drivers ]; then
	fail "Linux-shaped directories present"
fi

if [ -f include/nxu/exec.h ]; then
	fail "obsolete include/nxu/exec.h still present"
fi

if [ -d foundation/exec ]; then
	fail "obsolete foundation/exec still present"
fi

if ! grep -q 'nxu_cpu_exec_context' include/nxu/context.h; then
	fail "missing nxu_cpu_exec_context"
fi

if ! grep -q 'mov	x23, sp' silicon/aarch64/entry/start.S; then
	fail "entry does not capture incoming SP before replacing it"
fi
if ! grep -q 'nxu_boot_stack_top' silicon/aarch64/entry/start.S; then
	fail "entry does not install NXU stack"
fi
if ! grep -q 'msr	daifset, #0xf' silicon/aarch64/entry/start.S; then
	fail "entry does not mask DAIF"
fi

if ! grep -q 'nxu_s1_intake' foundation/enter.c; then
	fail "S1 not in enter sequence"
fi
if ! grep -q 'nxu_s14_freeze' foundation/enter.c; then
	fail "S14 not in enter sequence"
fi

if ! grep -q -- '-Iinclude' Makefile || ! grep -F -q -- '-I.' Makefile; then
	fail "Makefile missing project include roots"
fi

if ! grep -q -- '-MMD' Makefile; then
	fail "Makefile missing header dependency tracking"
fi

echo "check: source rules ok"

check_img() {
	img="$1"
	[ -f "$img" ] || return 0
	python3 - "$img" <<'PY' || fail "image header $1"
import struct, sys
p = sys.argv[1]
img = open(p, "rb").read()
if len(img) < 64:
    raise SystemExit("image too small")
if img[56:60] != b"ARM\x64":
    raise SystemExit("missing ARM64 magic")
insn0 = struct.unpack_from("<I", img, 0)[0]
if (insn0 >> 26) != 0x05:
    raise SystemExit("code0 is not B")
flags = struct.unpack_from("<Q", img, 24)[0]
if flags != 0xA:
    raise SystemExit("unexpected flags")
print("check: %s header ok, %d bytes" % (p, len(img)))
PY
}

check_img build-qemu/nxu.img
check_img build-fvp/nxu.img
check_img build-debug/nxu.img
