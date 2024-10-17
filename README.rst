**********************
Lab 6: Virtual² Memory
**********************

.. contents:: Contents
    :depth: 2

=====
TL;DR
=====

Deadlines
---------

- Tasks 1, 2: April 9th, 23:59 Vancouver time
- Tasks 3—5: April 13th, 23:59 Vancouver time


Summary
-------

In this lab, you will create a slightly simplified emulation of virtual memory system. Of course you won't be doing this inside a kernel, but most of the implementation is very similar.

Some simplifications have been made (in particular various limits) to make this (substantially) easier to implement than the real thing.


Logistics
---------

As in the prior lab, we will use ``castor.ece.ubc.ca`` to evaluate your submissions. We have tried to make the code work across different machines, but if your submission works on some machine you have but not on ``castor.ece.ubc.ca``, you will be out of luck.

As in the other labs, all submissions are made by committing your code and pushing the commits to the assignment repo on GitHub.

As with prior labs, tasks are cumulative.

As with all work in CPEN212, all of your code must be original and written by you from scratch after the lab has been released. You may not refer to any code other than what is found in the textbook and lectures, and you must not share code with anyone. See the syllabus for the academic integrity policy for details.


Advice
------

Unlike in the memory allocation lab, we are not providing any test cases for you. This means you will need to create test cases yourself. If you don't thoroughly test your code, it is unlikely that your code will pass our acceptance tests.

Be sure you automate tests rather than run them by hand. You want to run all of them whenever you make changes as a *regression testsuite* to make sure you haven't broken anything.

It's not a bad idea to write test cases based on the spec *before* implementing anything. This way more and more tests will pass as you implement various features.

You will almost certainly want to write some helper functions that pretty-print various things in a human-readable way: the page hierarchy, the set of all mapped pages, etc. This is even more important than in the allocation lab, since there are multi-level data structures here.

Once you have a swap file, you will want to be able to quickly go to an arbitrary location in the file; ``fseeko()`` is useful for this.


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

Swap space, if present, is at least 2 pages and no more than 67,108,864 pages.

The page table is a two-level hierarchy, with first-level and second-level page tables in addition to pages allocated for the application. To address them, a virtual address VA is split into the following bit ranges:

- VA[31:22] indexes the first-level page table,

- VA[21:12] indexes the second-level page table, and

- VA[11:0] is the page offset.

The system supports up to 1000 concurrent processes, each with their own page table hierarchy. Note that more processes may exist over time, but only 1000 must be supported concurrently.

A processes is uniquely identified by the physical address of its first-level page table.

All pages, including page table levels, are aligned on 4096-byte boundaries.


Operations
----------

The VM system supports four operations: memory accesses, mapping, unmapping, and eviction policy reset.

**Accesses** to virtual memory addresses may be of three types:

- read access,

- write access, and

- instruction fetch.

These accesses will fail unless the appropriate permission bits are set in the relevant page table entry.

Accesses may be made from user-level code or kernel-level code. User-level accesses will fail unless the relevant page table entry allows user accesses.

The **mapping** operation allocates a page in physical memory and maps it to a process's virtual address space, also creating any page tables that are required.

The **unmapping** operation removes a page from a process's virtual address space, also removing any page tables that have no valid entries. If a process's first-level page table is deallocated this way, the process is considered to no longer exist.

**Eviction policy resets** clear all accessed bits in all of the relevant process's PTEs.


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


Page faults
-----------

If swap space is present, pages in physical memory may be swapped out to disk. Both pages allocated to processes and page tables may be swapped out. The contents and permissions for evicted pages must be preserved when they are swapped back to physical memory.

Page faults occur in two scenarios:

- A PTE points to a page where valid = 1 but present = 0. This may occur during accesses, mapping, or unmapping. The relevant page is brought to physical memory, with another page swapped out only if no free pages remain in physical memory.

- A new page must be created during mapping (for either a page table or process-accessible page), but no free pages remain in physical memory. A page is swapped out, creating space for the new page.

Note that either of these cases up to three page faults may be caused by a single operation.

If eviction is required during a page fault and there are any pages with the accessed bit cleared, one of those pages must be selected for eviction. If all pages have the accessed bit set, any of these pages may be selected for eviction.

A page table is never swapped out unless all of the pages it points to have been swapped out.


Implementation requirements
---------------------------

The contents of any page allocated to any process's virtual memory space may not be altered by the VM system.

If the physical memory has N pages total, at least N-1 of those must be usable (either for intermediate page tables or the pages allocated to processes). For example, if there are 4 pages total, you must support a minimum of one allocatable page (in this case, two levels of PT and the page allocated for the application).

First- and second-level page tables that have no valid entries must be deallocated. For example, when physical memory has 4 pages, it must be possible to allocate one process-usable page, deallocate this page, and allocate another process-usable page for a separate process.

If the physical page has N pages total and the swap space has M pages total, the total number of usable pages in the entire system is (N - 1) + (M - 1). In particular, this means that page tables themselves must be evictable to swap space.


======
Coding
======

Templates
---------

For each task, we've provided a header file ``cpen212vm.h`` that defines the API to your implementation, and a skeleton ``cpen212vm.c`` file where you will fill in your implementation. The templates are the same for all tasks.


Rules
-----

Some rules you must obey when writing code:

- When compiling your code, we will only use ``cpen212vm.c`` in the relevant directory; we will use a fresh copy of ``cpen212vm.h``. This means that all your code must be in ``cpen212vm.c``.

- You may define whatever additional functions you like, provided they are not visible in the global linker namespace (i.e., they are declared as ``static``).

- You may not use global variables (even if they are ``static``).

- You may not allocate any memory (e.g., using ``malloc``) beyond the physical memory range provided to ``vm_init()``.

- You may not use any names that start with a double underscore (e.g., ``__foo``).

- Your code must be in C (specifically ISO C17).

- Your code must not require linking against any libraries other that the usual ``libc`` (which is linked against by default when compiling C).

- Needless to say, your code must compile and run without errors. If we can't compile or run your code, you will receive no credit for the relevant task.

If you violate these rules, we will likely not be able to compile and/or properly test your code.


======
Task 1
======

Requirements
------------

Required functionality:

- correct translation for accesses to virtual addresses mapped to physical memory

- all permission checks and suitable failures


Deliverables
------------

- ``cpen212vm.c`` in the ``task1`` directory


======
Task 2
======

Requirements
------------

Required functionality in addition to previous tasks:

- mapping new pages provided there are enough free physical memory pages

- creating any first-level and second-level page tables necessary to complete the mapping

- relevant failures

- removal of any page tables created if the mapping ultimately fails


Deliverables
------------

- ``cpen212vm.c`` in the ``task2`` directory


======
Task 3
======

Requirements
------------

- unmapping existing pages provided all levels are resident in physical memory

- relevant failures

- removal of any page tables that have no valid entries


Deliverables
------------

- ``cpen212vm.c`` in the ``task3`` directory



======
Task 4
======

Requirements
------------

- retrieving process-allocated pages from the swap file during page faults

- evicting process-allocated pages to the swap file during page faults

- eviction candidates may be limited to the process-allocated pages belonging to process initiating the access or the mapping (in this task only)

- relevant failures


Deliverables
------------

- ``cpen212vm.c`` in the ``task4`` directory



======
Task 5
======

Requirements
------------

- first-level and second-level page tables are evictable

- eviction candidates may come from any existing process

- relevant failures


Deliverables
------------

- ``cpen212vm.c`` in the ``task5`` directory



=====
Marks
=====

- Task 1: 2
- Task 2: 2
- Task 3: 2
- Task 4: 2
- Task 5: 2
