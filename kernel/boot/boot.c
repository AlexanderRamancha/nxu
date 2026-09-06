#include <nxu/boot.h>
#include <nxu/dtb.h>
#include <nxu/log.h>


int
nxu_boot_validate(const struct nxu_boot_info *boot_info)
{
    if (boot_info == 0)
        return -1;

    if (boot_info->dtb_address == 0)
        return -1;

    (void)nxu_log_event(
        &(struct nxu_log_event){
            .subsystem = NXU_LOG_SUBSYSTEM_BOOT,
            .code = NXU_LOG_EVENT_BOOT_VALIDATED
        },
        0,
        0
    );

    return 0;
}