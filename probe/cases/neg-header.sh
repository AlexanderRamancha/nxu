#!/bin/sh
# Negative test: corrupt ARM64 header magic. Must not freeze.
set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
src="$root/build-qemu/nxu.img"
bad="$root/build-qemu/nxu.bad-header.img"
qemu="$root/probe/host/qemu/bin/qemu-system-aarch64"
out="$root/probe/evidence/qemu-neg-header.log"

if [ ! -f "$src" ]; then
	echo "missing $src" >&2
	exit 1
fi
mkdir -p "$root/probe/evidence"
cp "$src" "$bad"
python3 - "$bad" <<'PY'
import sys
p = sys.argv[1]
with open(p, "r+b") as f:
    f.seek(56)
    f.write(b"XXXX")
PY

if [ ! -x "$qemu" ]; then
	qemu=qemu-system-aarch64
fi

timeout 4 "$qemu" \
	-machine virt,gic-version=3 \
	-cpu cortex-a57 \
	-smp 1 \
	-m 128M \
	-nographic \
	-kernel "$bad" \
	> "$out" 2>&1 || true

if grep -q '\[NXU\] The first NXU application has completed.' "$out"; then
	echo "NEG header FAIL: kernel event after bad magic" >&2
	exit 1
fi
echo "NEG header ok: no kernel event after bad magic"
