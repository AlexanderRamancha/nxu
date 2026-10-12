#ifndef NXU_CONSOLE_H
#define NXU_CONSOLE_H

/*
 * NXU MAP
 *
 *                    log record
 *                        │
 *                        ▼
 *                     console
 *                        │
 *                        ▼
 *                      sink
 *
 * Layer: foundation observe
 * Owns: one-line presentation of a log record
 * Consumes: record; sink bind
 * Produces: text after the sink is bound
 * Hardware: none
 * Forbidden: print from a decision path; print before bind
 * Invariant: no sink, or not CPU 0 → silent return
 */

#include <nxu/types.h>

struct nxu_log_record;

void nxu_console_record(const struct nxu_log_record *r);
void nxu_debug(const char *tag, const char *msg);
void nxu_debug_u64(const char *key, nxu_u64 v);

#endif
