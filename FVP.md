# FVP Base RevC

Do not run this tree from Trash. Extract to a real directory:

    rm -rf "$HOME/Alex/myOS/nxu"
    mkdir -p "$HOME/Alex/myOS"
    unzip -o nxu-latest.zip -d "$HOME/Alex/myOS"
    cd "$HOME/Alex/myOS/nxu"

    export FVP="$HOME/fvp/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA"
    make clean
    make fvp
    make fvp-run

The model maps BL1 ROM at 0x0. QEMU uses the VA-0 image, but FVP uses the
DRAM-linked image at 0x80000000. The probe loads the FVP raw image with:

The default launcher uses the verified Fast Models 11.29.27 installation
(`Base_RevC_AEMvA_pkg`) and falls back to the older unversioned
installation if it is still present. Set `FVP=/path/to/model` to override it.

    --data FVP_Base_RevC_2xAEMvA.cluster0.cpu0=build-fvp/nxu.img@0x80000000
    -C bp.secure_memory=false
    -C bp.refcounter.base_frequency=100000000
    -C cluster0.cpu0.RVBAR=0x80000000
    -C pctl.startup=0.0.0.0
    --start FVP_Base_RevC_2xAEMvA.cluster0.cpu0=0x80000000
    -C cluster0.has_el3=true
    -C bp.pl011_uart0.out_file=probe/evidence/fvp-uart.log

UART is FVP PL011 at 0x1C090000 (board fact only). Console is captured in
`probe/evidence/fvp-uart.log`. Telnet "Listening on port 5000" lines from the
model are not NXU output.

For an interactive terminal, run `make fvp-run` and use the terminal port
reported by the model (the updated 11.29.27 model reports terminal 0 on port
5000). The interactive target keeps FVP running; `make fvp` remains the
bounded evidence probe.

`make fvp` builds only. `make fvp-run` builds and launches the model.
