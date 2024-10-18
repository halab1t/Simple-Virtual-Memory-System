
=============
Specification
=============

VM configuration
----------------

The memory system parameters you need to implement has the following parameters:

- virtual addresses are 32 bits,

- physical addresses are 32 bits, and

- page size is 4096 bytes.

Physical memory contains at least 4 pages and no more than 1,048,576 pages.

The page table is a two-level hierarchy, with first-level and second-level page tables in addition to pages allocated for the application. To address them, a virtual address VA is split into the following bit ranges:

- VA[31:22] indexes the first-level page table,

- VA[21:12] indexes the second-level page table, and

- VA[11:0] is the page offset.

A processes is uniquely identified by the physical address of its first-level page table.

All pages, including page table levels, are aligned on 4096-byte boundaries.


Operations
----------

The VM system supports four operations: memory accesses and  mapping. Unmapping and eviction policy reset are not yet implemented.

**Accesses** to virtual memory addresses may be of three types:

- read access,

- write access, and

- instruction fetch.

The **mapping** operation allocates a page in physical memory and maps it to a process's virtual address space, also creating any page tables that are required.


Page table entries
------------------

Each page table entry is 32 bits; when stored in memory or on disk, the 32-bit value appears in little-endian byte order.

Page table entries (PTEs) comprise the following bitfields:

- PTE[31:12] (ppn) physical page number
- PTE[11:7] (reserved) reserved for implementation use
- PTE[6] (accessed) indicates that this page was accessed since the last time accessed bits were reset
- PTE[5] (user) indicates that user-level accesses to this page are permitted (otherwise kernel-level only)
- PTE[4] (executable) indicates that instruction fetches from this page are permitted
- PTE[3] (writable) indicates that writes to this page are permitted
- PTE[2] (readable) indicates that reads from this page are permitted
- PTE[1] (present) indicates that this page is present in physical memory
- PTE[0] (valid) indicates that this page is mapped in the process's virtual address space

In PTEs where valid = 0, all other bits are reserved for the implementation.

In PTEs where valid = 1 and present = 0, bits [31:6] are reserved for the implementation.

In PTEs that identify a next-level page table, the four permission bits (user/executable/writable/readable) are reserved for the implementation.





