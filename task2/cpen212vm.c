#include "cpen212vm.h"

#define BYTES_PER_PG 4096
#define NUM_PTE_PER_PG 1024
//common extractions definitions
#define NUM_BITS_PERMISSIONS 1
#define NUM_BITS_PPN 20
#define NUM_BITS_PT_INDEX 10
#define NUM_BITS_PAGE_OFFSET 12
#define START_BIT_PPN 12
#define START_BIT_PT1_INDEX 22
#define START_BIT_PT2_INDEX 12
#define START_BIT_PAGE_OFFSET 0



typedef struct {
  bool active;
  paddr_t pt;
} process;

typedef struct {
  void *start_addr; //first page of virtual mem
  size_t num_usable_pages; // pages of virtual memory
  int num_active_processes;
} vm_t;

typedef enum { //types of permissions, the number they are assigned represents the bit in the pte where you can find this permission
  VALID = 0,
  PRESENT = 1,
  READABLE = 2,
  WRITABLE = 3,
  EXECUTABLE = 4,
  USER = 5,
  ACCESSED = 6,
} permission_t;

static uint32_t bit_extraction(int start_bit, int num_bits, uint32_t num){
	return (uint32_t)(((1 << num_bits) - 1) & (num >> (start_bit)));
}

static uint32_t pte_extraction(void* start_addr, paddr_t pt, uint32_t page_index){
	void* pte_addr = (void *)((char *)start_addr + (size_t)pt + (size_t)page_index);
	uint32_t* pte = (uint32_t*)pte_addr;
	return *pte;
}

static bool check_permissions(permission_t permission_type, uint32_t pte){
	int start_bit = (int)permission_type;
	bool permission_granted = bit_extraction(start_bit, NUM_BITS_PERMISSIONS, pte);
	return permission_granted;
}
//only to be used if accessed bit is zero
static void set_perm_bit(void* start_addr, paddr_t pt, uint32_t page_index, permission_t perm){
	int bit_num = (int)perm;
	void* pte_addr = (void *)((char *)start_addr + (size_t)pt + (size_t)page_index);
	uint32_t* pte = (uint32_t*)pte_addr;
	*pte |= 1 << bit_num;
  	return;	
}

static void clear_perm_bit(void* pte, permission_t perm){
	int bit_num = (int)perm;
	uint32_t* pt_entry = (uint32_t*)pte;
	*pt_entry &= ~(1 << bit_num);
	return;	
}

void *vm_init(void *phys_mem, size_t num_phys_pages, FILE *swap, size_t num_swap_pages) {
	vm_t* virtual_mem = (vm_t *)phys_mem;
	virtual_mem->start_addr = (void *)phys_mem; 
	virtual_mem->num_usable_pages = num_phys_pages - 1;
	virtual_mem->num_active_processes = 0;
    	return (void *) phys_mem;
}

void vm_deinit(void *vm) {
    // TODO
}

vm_result_t vm_map_page(void *vm, bool new_process, paddr_t pt, vaddr_t addr,
                        bool user, bool exec, bool write, bool read) {
	
	vm_t* virtual_mem = (vm_t*)vm;
	if(new_process){}
}

vm_status_t vm_unmap_page(void *vm, paddr_t pt, vaddr_t addr) {
    // TODO
}

vm_result_t vm_translate(void *vm, paddr_t pt, vaddr_t addr, access_type_t acc, bool user) {
	//TODO: first translate the addr to the 2nd vt and check permissions 
	vm_t* virtual_memory = (vm_t *) vm;
	uint32_t pt1_index = bit_extraction(START_BIT_PT1_INDEX, NUM_BITS_PT_INDEX, addr);
	uint32_t pt2_index = bit_extraction(START_BIT_PT2_INDEX, NUM_BITS_PT_INDEX, addr);
	uint32_t page_offset = bit_extraction(START_BIT_PAGE_OFFSET, NUM_BITS_PAGE_OFFSET, addr);
	uint32_t pte_1 = pte_extraction(virtual_memory->start_addr, pt, pt1_index);
	uint32_t ppn_pt2 = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, pte_1);
	paddr_t pt2 = BYTES_PER_PG*ppn_pt2;
	uint32_t pte_2 = pte_extraction(virtual_memory->start_addr, pt2, pt2_index);
	bool inaddequate_permissions = false;
	if(check_permissions(VALID, pte_2)&&check_permissions(PRESENT, pte_2)){
	    if(!check_permissions(ACCESSED, pte_2)){
	    	set_perm_bit(virtual_memory->start_addr, pt2, pt2_index, ACCESSED);
	    }		
	    if(user){
	    	if(!check_permissions(USER, pte_2)){
		    inaddequate_permissions = true;
		}
	    }
	    switch (acc){
	    	case VM_EXEC: 
	    		if(!check_permissions(EXECUTABLE, pte_2)){
		    	    inaddequate_permissions = true;
			} 
			break;      
	        case VM_READ: 
	    		if(!check_permissions(READABLE, pte_2)){
		    	    inaddequate_permissions = true;
			} 
			break;      
			      
	        case VM_WRITE: 
	    		if(!check_permissions(WRITABLE, pte_2)){
		    	    inaddequate_permissions = true;
			} 
			break;    
	    }
	    if(inaddequate_permissions){
		vm_result_t result = {VM_BAD_PERM, 0};
		return result; 
	    }
	}else{
		vm_result_t result = {VM_BAD_ADDR, 0};
		return result;
	}
	uint32_t ppn = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, pte_2);
	paddr_t result_addr = BYTES_PER_PG*ppn;
	vm_result_t result = {VM_OK, result_addr};
	return result;
}

void vm_reset_accessed(void *vm, paddr_t pt) {
    vm_t* virtual_memory = (vm_t*)vm;
    uint32_t* pte_1 = (uint32_t*)((char *)virtual_memory->start_addr + (size_t)pt);
    for(int pt1_index = 0; pt1_index < NUM_PTE_PER_PG; pt1_index++){
	uint32_t ppn_pt2 = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, *pte_1);
	paddr_t pt2 = ppn_pt2*BYTES_PER_PG;
	uint32_t* pte_2 = (uint32_t*)((char*)virtual_memory->start_addr + (size_t)pt2);
        for(int pt2_index = 0; pt2_index < NUM_PTE_PER_PG; pt2_index++){
	    clear_perm_bit(pte_2, ACCESSED);
	    pte_2++;
	}
	pte_1++;
    }
    return;
}

