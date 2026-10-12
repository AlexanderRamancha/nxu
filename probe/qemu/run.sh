#!/bin/sh
#
# NXU QEMU virt probe
#
# Executes NXU under QEMU and captures console output through a PTY.
# Success requires:
#   [NXU] NXU kernel execution established
# and no:
#   [NXU] Halted
#

set -u

root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd) || exit 1

img="${NXU_IMG:-$root/build-qemu/nxu.img}"
smp="${NXU_SMP:-4}"
out="${NXU_LOG:-$root/probe/evidence/qemu.log}"

if [ -n "${NXU_QEMU:-}" ]; then
    qemu="$NXU_QEMU"
elif [ -x "$root/probe/host/qemu/bin/qemu-system-aarch64" ]; then
    qemu="$root/probe/host/qemu/bin/qemu-system-aarch64"
else
    qemu="$(command -v qemu-system-aarch64 2>/dev/null || true)"
fi

if [ ! -f "$img" ]; then
    echo "QEMU launch FAIL: image not found:"
    echo "  $img"
    exit 1
fi

if [ -z "$qemu" ]; then
    echo "QEMU launch FAIL: qemu-system-aarch64 not found"
    exit 1
fi

if [ ! -x "$qemu" ]; then
    echo "QEMU launch FAIL: QEMU is not executable:"
    echo "  $qemu"
    exit 1
fi

case "$smp" in
    ''|*[!0-9]*|0)
        echo "QEMU launch FAIL: invalid NXU_SMP=$smp"
        exit 1
        ;;
esac

mkdir -p "$(dirname "$out")" || exit 1
rm -f "$out"

echo "QEMU: $qemu"
echo "image: $img"
echo "smp:   $smp"
echo "log:   $out"

python3 - "$qemu" "$img" "$smp" "$out" <<'PY'
import os
import pty
import select
import subprocess
import sys
import time

qemu, image, smp, output = sys.argv[1:]

cmd = [
    qemu,
    "-machine", "virt,gic-version=3",
    "-cpu", "cortex-a57",
    "-smp", smp,
    "-m", "128M",
    "-nographic",
    "-kernel", image,
]

master, slave = pty.openpty()

proc = subprocess.Popen(
    cmd,
    stdin=slave,
    stdout=slave,
    stderr=slave,
    close_fds=True,
)

os.close(slave)

deadline = time.time() + 8.0
seen = bytearray()

with open(output, "wb") as log:
    while True:
        if time.time() >= deadline:
            proc.terminate()
            break

        ready, _, _ = select.select([master], [], [], 0.1)

        if master not in ready:
            if proc.poll() is not None:
                break
            continue

        try:
            data = os.read(master, 4096)
        except OSError:
            break

        if not data:
            if proc.poll() is not None:
                break
            continue

        log.write(data)
        log.flush()

        seen.extend(data)

        # Show the same output live.
        sys.stdout.buffer.write(data)
        sys.stdout.buffer.flush()

        # The kernel event is the intended terminal state.
        if b"[NXU] The first NXU application has completed." in seen:
            time.sleep(0.2)
            proc.terminate()
            break

try:
    rc = proc.wait(timeout=2)
except subprocess.TimeoutExpired:
    proc.kill()
    rc = proc.wait(timeout=2)

os.close(master)

print("QEMU process rc=%s" % rc)

if b"[NXU] The first NXU application has completed." not in seen:
    sys.exit(2)

if b"[NXU] Halted" in seen:
    sys.exit(3)

sys.exit(0)
PY

rc=$?

echo "QEMU probe rc: $rc"

if [ ! -s "$out" ]; then
    echo "QEMU capture FAIL: evidence log is empty"
    exit 1
fi

if ! grep -qF '[NXU] NXU kernel execution is established.' "$out"; then
    echo "QEMU capture FAIL: no kernel event"
    echo "---- qemu output ----"
    tail -n 60 "$out"
    exit 1
fi

if grep -qF '[NXU] Halted' "$out"; then
    echo "QEMU capture FAIL: halt event present"
    grep -F '[NXU] Halted' "$out"
    exit 1
fi

if ! grep -qF '[NXU] The first NXU application has completed.' "$out"; then
    echo "QEMU capture FAIL: application did not complete"
    exit 1
fi

echo "QEMU capture ok: kernel event observed (smp=$smp)"
echo "log: $out"

exit 0
