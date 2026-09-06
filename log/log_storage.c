#include <nxu/log.h>
#include <nxu/cpu.h>

/*
 * NXU per-CPU diagnostic storage.
 *
 * Ownership model:
 *
 *     CPU owns slot
 *          |
 *        write
 *          |
 *       finalize
 *          |
 *     consumer owns slot
 *
 * A producer never modifies a finalized slot.
 */

#define NXU_LOG_STORAGE_CAPACITY 16U

enum nxu_log_slot_state {
    NXU_LOG_SLOT_FREE = 0,
    NXU_LOG_SLOT_WRITING,
    NXU_LOG_SLOT_READY
};

struct nxu_log_slot {
    volatile nxu_u8 state;
    struct nxu_log_record record;
};

struct nxu_log_cpu_storage {
    struct nxu_log_slot slots[NXU_LOG_STORAGE_CAPACITY];

    nxu_u32 write_index;
    nxu_u32 read_index;
};

/*
 * One independent storage region per possible CPU.
 *
 * No CPU shares producer-owned mutable storage with
 * another CPU.
 */
static struct nxu_log_cpu_storage
nxu_log_storage[NXU_MAX_CPUS];

void
nxu_log_storage_init(void)
{
    nxu_u32 cpu_id;
    nxu_u32 slot_id;

    for (cpu_id = 0U;
         cpu_id < NXU_MAX_CPUS;
         cpu_id++) {

        nxu_log_storage[cpu_id].write_index = 0U;
        nxu_log_storage[cpu_id].read_index = 0U;

        for (slot_id = 0U;
             slot_id < NXU_LOG_STORAGE_CAPACITY;
             slot_id++) {

            nxu_log_storage[cpu_id]
                .slots[slot_id]
                .state = NXU_LOG_SLOT_FREE;
        }
    }
}

/*
 * Reserve a slot for the current CPU.
 *
 * The caller receives exclusive ownership of the slot
 * until it is finalized.
 */
struct nxu_log_record *
nxu_log_storage_reserve(void)
{
    nxu_u32 cpu_id;
    struct nxu_log_cpu_storage *storage;
    struct nxu_log_slot *slot;

    cpu_id = nxu_cpu_current_id();

    if (cpu_id >= NXU_MAX_CPUS)
        return 0;

    storage = &nxu_log_storage[cpu_id];

    slot = &storage->slots[storage->write_index];

    if (slot->state != NXU_LOG_SLOT_FREE)
        return 0;

    slot->state = NXU_LOG_SLOT_WRITING;

    return &slot->record;
}

/*
 * Finalize the current CPU's reserved slot.
 *
 * Once READY, the producer must no longer modify the record.
 */
int
nxu_log_storage_commit(void)
{
    nxu_u32 cpu_id;
    struct nxu_log_cpu_storage *storage;
    struct nxu_log_slot *slot;

    cpu_id = nxu_cpu_current_id();

    if (cpu_id >= NXU_MAX_CPUS)
        return -1;

    storage = &nxu_log_storage[cpu_id];

    slot = &storage->slots[storage->write_index];

    if (slot->state != NXU_LOG_SLOT_WRITING)
        return -1;

    asm volatile(
    "stlrb %w0, [%1]"
    :
    : "r"((nxu_u32)NXU_LOG_SLOT_READY),
      "r"(&slot->state)
    : "memory");
    
    storage->write_index++;

    if (storage->write_index >= NXU_LOG_STORAGE_CAPACITY)
        storage->write_index = 0U;

    return 0;
}
