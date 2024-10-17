#include "cpen212vm.h"

void *vm_init(void *phys_mem, size_t num_phys_pages, FILE *swap, size_t num_swap_pages) {
    // TODO
}

void vm_deinit(void *vm) {
    // TODO
}

vm_result_t vm_map_page(void *vm, bool new_process, paddr_t pt, vaddr_t addr,
                        bool user, bool exec, bool write, bool read) {
    // TODO
}

vm_status_t vm_unmap_page(void *vm, paddr_t pt, vaddr_t addr) {
    // TODO
}

vm_result_t vm_translate(void *vm, paddr_t pt, vaddr_t addr, access_type_t acc, bool user) {
    // TODO
}

void vm_reset_accessed(void *vm, paddr_t pt) {
    // TODO
}

