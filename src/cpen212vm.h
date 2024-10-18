#ifndef __CPEN212VM_H__
#define __CPEN212VM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef uint32_t vaddr_t; // virtual address
typedef uint32_t paddr_t; // physical address

typedef enum {
    VM_EXEC = 0, // instruction fetch
    VM_READ = 1, // data read
    VM_WRITE = 2 // data write
} access_type_t;

typedef enum {
    VM_OK = 0,         // success
    VM_BAD_ADDR = 1,   // virtual address not mapped
    VM_BAD_PERM = 2,   // insufficient permissions for access
    VM_OUT_OF_MEM = 3, // out of physical memory / swap
    VM_DUPLICATE = 4,  // request to map an existing page
    VM_BAD_IO = 5      // I/O operation failed
} vm_status_t;

typedef struct {
    vm_status_t status; // translation outcome
    paddr_t addr;       // translated physical address, relevant only if status is VM_OK
} vm_result_t;

// description:
// - initializes a VM system
// arguments:
// - physmem: pointer to an area of at least 4096 * num_phys_pages bytes that models the physical memory
//   - no vm_* functions may access memory outside of [physmem, physmem+4096*num_phys_pages)
//   - all physical addresses are offsets from physmem (i.e., physical address is exactly 0 at physmem)
// - num_phys_pages: total number of 4096-byte physical pages available
//   - it is guaranteed that 4 <= num_phys_pages <= 1048576
//   - physical page 0 starts at physmem
// - swap
//   - if non-null: pointer to a swap file opened in read-write mode with size 4096 * num_swap_pages bytes
//   - if null: no swap space is available for this VM instance
// - num_swap_pages: total number of 4096-byte pages available in the swap file
//   - only relevant if swap is not null
//   - if swap is non-null, it is guaranteed that 2 <= num_swap_pages <= 67108864
// returns:
// - a handle that uniquely identifies this VM instance; this will be passed unchanged to other vm_* functions
//   multiple VM instances may co-exist
void *vm_init(void *physmem, size_t num_phys_pages, FILE *swap, size_t num_swap_pages);

// description:
// - deinitializes a VM system
// - note that any swap files are *not* closed
// arguments:
// - vm: a VM system handle returned from vm_init
void vm_deinit(void *vm);

// description:
// - translates a virtual address to a physical address if possible
// arguments:
// - vm: a VM system handle returned from vm_init
// - pt: physical address of the top-level page table of the accessing process
// - addr: the virtual address to translate
// - access: the access being made (instruction fetch, read, or write)
// - user: the access is a user-level access (i.e., not a kernel access)
// returns:
// - the success status of the translation:
//   - VM_OK if translation succeeded
//   - VM_BAD_ADDR if there is no translation for this address
//   - VM_BAD_PERM if permissions are not sufficient for the type / source of access requested
//   - VM_BAD_IO if accessing the swap file failed
// - the resulting physical address (relevant only if status is VM_OK)
vm_result_t vm_translate(void *vm, paddr_t pt, vaddr_t addr, access_type_t access, bool user);

// description:
// - creates a mapping for a new page in the virtual address space and map it to a physical page
// arguments:
// - vm: a VM system handle returned from vm_init
// - new_process: true iff there is no top-level page table for this process
//   - there may be up to 1000 (inclusive) separate processes with active top-level page tables
// - pt: physical address of the top-level page table of the process (relevant only if new_process is false)
// - addr: the virtual address on a page that is to be mapped (not necessarily the start of the page)
// - user: the page is accessible from user-level processes
// - exec: instructions may be fetched from this page
// - write: data may be written to this page
// - read: data may be read from this page
// returns:
// - the success status of the translation:
//   - VM_OK if the mapping succeeded
//   - VM_OUT_OF_MEM if no free pages remain in the physical memory and any relevant swap
//   - VM_OUT_OF_MEM if new_process is true and there are already 1000 active processes with top-level page tables
//   - VM_DUPLICATE if a mapping for this page already exists
//   - VM_BAD_IO if accessing the swap file failed
// - the physical address of the *top-level* page table for this process (relevant only if status is VM_OK)
vm_result_t vm_map_page(void *vm, bool new_process, paddr_t pt, vaddr_t addr,
                        bool user, bool exec, bool write, bool read);

// description:
// - removes the mapping for the page that contains virtual address addr
// - returns any unmapped pages and any page tables with no mappings to the free page pool
// arguments:
// - vm: a VM system handle returned from vm_init
// - pt: physical address of the top-level page table of the process
// - addr: the virtual address on a page that is to be unmapped (not necessarily the start of the page)
// returns:
// - the success status of the translation:
//   - VM_OK if the page was successfully unmapped
//   - VM_BAD_ADDR if this process has no mapping for virtual address addr
//   - VM_BAD_IO if accessing the swap file failed
vm_status_t vm_unmap_page(void *vm, paddr_t pt, vaddr_t addr);

// description:
// - resets the used bit for all pages used by the specified process in physical memory
// arguments:
// - vm: a VM system handle returned from vm_init
// - pt: physical address of the top-level page table of the process
void vm_reset_accessed(void *vm, paddr_t pt);

#endif // __CPEN212VM_H__

