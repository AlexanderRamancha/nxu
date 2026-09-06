#ifndef NXU_LOG_STORAGE_H
#define NXU_LOG_STORAGE_H

#include <nxu/log.h>

void
nxu_log_storage_init(void);

struct nxu_log_record *
nxu_log_storage_reserve(void);

int
nxu_log_storage_commit(void);

#endif