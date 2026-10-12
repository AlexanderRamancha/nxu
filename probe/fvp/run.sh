#!/bin/sh

set -u

ROOT="$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)"

if [ -z "${FVP:-}" ]; then
    for candidate in \
        "$HOME/fvp/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA" \
        "$HOME/fvp/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA"
    do
        if [ -x "$candidate" ]; then
            FVP="$candidate"
            break
        fi
    done
    FVP="${FVP:-$HOME/fvp/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA}"
fi
IMG="${NXU_IMG:-$ROOT/build-fvp/nxu.img}"

LOAD="${NXU_FVP_LOAD:-0x80000000}"
# Base RevC is two clusters. The model allows at most four cores in each.
# NXU_SMP remains an optional cap for a smaller run. The default exercises
# both clusters. The kernel still has to discover the CPUs; this only
# exposes them.
CLUSTER0="${NXU_FVP_CLUSTER0:-4}"
CLUSTER1="${NXU_FVP_CLUSTER1:-4}"
if [ -n "${NXU_SMP:-}" ]; then
    CLUSTER0="$NXU_SMP"
    CLUSTER1=0
fi
CPU_INSTANCE="${NXU_FVP_CPU_INSTANCE:-FVP_Base_RevC_2xAEMvA.cluster0.cpu0}"

UART_LOG="${NXU_UART_LOG:-$ROOT/probe/evidence/fvp-uart.log}"
MODEL_LOG="${NXU_MODEL_LOG:-$ROOT/probe/evidence/fvp-model.log}"

TIMEOUT="${NXU_FVP_TIMEOUT:-40}"
INTERACTIVE="${NXU_FVP_INTERACTIVE:-0}"

mkdir -p "$ROOT/probe/evidence"

rm -f "$UART_LOG" "$MODEL_LOG"

echo
echo "========================================"
echo " NXU FVP"
echo "========================================"
echo
echo "FVP       : $FVP"
echo "IMAGE     : $IMG"
echo "LOAD      : $LOAD"
echo "CLUSTER0  : $CLUSTER0"
echo "CLUSTER1  : $CLUSTER1"
echo "CPU       : $CPU_INSTANCE"
echo "UART LOG  : $UART_LOG"
echo "MODEL LOG : $MODEL_LOG"
echo

if [ ! -x "$FVP" ]; then
    echo "ERROR: FVP executable not found:"
    echo "$FVP"
    exit 1
fi

if [ ! -f "$IMG" ]; then
    echo "ERROR: NXU image not found:"
    echo "$IMG"
    exit 1
fi

case "$CLUSTER0" in
    1|2|3|4) ;;
    *) echo "ERROR: cluster 0 core count must be 1..4"; exit 1 ;;
esac
case "$CLUSTER1" in
    0|1|2|3|4) ;;
    *) echo "ERROR: cluster 1 core count must be 0..4"; exit 1 ;;
esac

echo "----------------------------------------"
echo " FVP CONFIGURATION"
echo "----------------------------------------"

set -- \
    -C "bp.secure_memory=false" \
    -C "cache_state_modelled=0" \
    -C "bp.refcounter.non_arch_start_at_default=1" \
    -C "bp.refcounter.base_frequency=100000000" \
    -C "bp.vis.disable_visualisation=1" \
    -C "cluster0.NUM_CORES=$CLUSTER0" \
    -C "cluster1.NUM_CORES=$CLUSTER1" \
    -C "cluster0.has_el3=true" \
    -C "cluster1.has_el3=true" \
    -C "cluster0.gicv3.SRE-EL2-enable-RAO=1" \
    -C "cluster0.gicv3.SRE-EL3-enable-RAO=1" \
    -C "cluster0.gicv3.SRE-EL3-set-once=1" \
    -C "cluster1.gicv3.SRE-EL2-enable-RAO=1" \
    -C "cluster1.gicv3.SRE-EL3-enable-RAO=1" \
    -C "cluster1.gicv3.SRE-EL3-set-once=1" \
    -C "gic_distributor.GICR-invalidate-registers-implemented=1" \
    -C "gic_distributor.delay-redistributor-accesses=0" \
    -C "gic_distributor.wakeup-on-reset=1" \
    -C "pctl.startup=0.0.0.0" \
    -C "bp.pl011_uart0.uart_enable=true" \
    -C "bp.pl011_uart0.untimed_fifos=true" \
    -C "bp.pl011_uart0.unbuffered_output=true" \
    -C "bp.pl011_uart0.out_file=$UART_LOG" \
    -C "bp.terminal_0.start_telnet=true" \
    -C "bp.terminal_1.start_telnet=true" \
    -C "bp.terminal_2.start_telnet=true" \
    -C "bp.terminal_3.start_telnet=true" \
    --data "$CPU_INSTANCE=$IMG@$LOAD" \
    --start "$CPU_INSTANCE=$LOAD"

c=0
while [ "$c" -lt "$CLUSTER0" ]; do
    set -- "$@" -C "cluster0.cpu${c}.RVBAR=$LOAD"
    c=$((c + 1))
done
c=0
while [ "$c" -lt "$CLUSTER1" ]; do
    set -- "$@" -C "cluster1.cpu${c}.RVBAR=$LOAD"
    c=$((c + 1))
done

echo "FVP:"
printf '  %s\n' "$FVP"

echo
echo "ARGS:"
printf '  %s\n' "$@"

echo
echo "----------------------------------------"
echo " STARTING FVP"
echo "----------------------------------------"
echo

rc=0

if [ "$INTERACTIVE" = "1" ]; then
    "$FVP" "$@" 2>&1 | tee "$MODEL_LOG"
    rc=$?
else
    timeout "$TIMEOUT" "$FVP" "$@" >"$MODEL_LOG" 2>&1 || rc=$?
fi

if [ "$INTERACTIVE" = "1" ]; then
    exit "$rc"
fi

echo
echo "FVP return code: $rc"

echo
echo "========================================"
echo " UART OUTPUT"
echo "========================================"

if [ -s "$UART_LOG" ]; then
    cat "$UART_LOG"
else
    echo "(empty)"
fi

echo
echo "========================================"
echo " MODEL OUTPUT"
echo "========================================"

if [ -s "$MODEL_LOG" ]; then
    cat "$MODEL_LOG"
else
    echo "(empty)"
fi

echo
echo "========================================"
echo " RESULT"
echo "========================================"

if grep -qF '[NXU] Halted' "$UART_LOG" 2>/dev/null; then
    echo "FVP capture FAIL: halt event present"
    echo
    echo "UART log : $UART_LOG"
    exit 1
fi

if grep -qF '[NXU] NXU kernel execution is established.' "$UART_LOG" 2>/dev/null \
    && grep -qF '[NXU] The first NXU application has completed.' "$UART_LOG" 2>/dev/null; then
    echo "FVP capture ok: application completed"
    echo
    echo "UART log : $UART_LOG"
    echo "Model log: $MODEL_LOG"
    exit 0
fi

if grep -qF 'terminal_0: Listening' "$MODEL_LOG" 2>/dev/null; then
    echo "FVP model started."
fi

echo "FVP capture FAIL: application did not complete."

echo
echo "This test used:"
echo "  RVBAR = $LOAD"
echo "  --data = nxu.img@$LOAD"
echo "  PL011 output file = $UART_LOG"
echo "  clusters = $CLUSTER0+$CLUSTER1"
echo
echo "UART log : $UART_LOG"
echo "Model log: $MODEL_LOG"

exit 1
