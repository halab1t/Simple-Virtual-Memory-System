#include "cpen212vm.h"

#define BYTES_PER_PG 4096
#define NUM_PTE_PER_PG 1024
//common extractions definitions
#define NUM_BITS_PERMISSIONS 1
#define NUM_BITS_PPN 20
#define NUM_BITS_PT_INDEX 10
#define NUM_BITS_PAGE_OFFSET 12
#define START_BIT_PPN 12
#define END_BIT_PPN_EXCLUSIVE 32
#define START_BIT_PT1_INDEX 22
#define START_BIT_PT2_INDEX 12
#define START_BIT_PAGE_OFFSET 0
#define NUM_FREE_PAGES_NEW_PROCESS 3


typedef struct {
  bool active;
  paddr_t pt;
} process;

typedef struct {
  void *start_addr; //first page of virtual mem
  size_t num_pages; // pages of virtual memory
  int num_active_processes;
  int num_free_pages;
  uint32_t free_page;
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

static uint32_t* pte_extraction(void* start_addr, paddr_t pt, uint32_t page_index){
	void* pte_addr = (void *)((char *)start_addr + (size_t)pt + (size_t)page_index);
	uint32_t* pte = (uint32_t*)pte_addr;
	return pte;
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

static void set_free_page_header(uint32_t page_num, uint32_t header, vm_t* virtual_mem){
	uint32_t* header_ptr = (void*)((char*)virtual_mem->start_addr+ (size_t)(page_num*BYTES_PER_PG));
	*header_ptr = header;
	return;
}

static bool is_in_free_list(vm_t* virtual_mem, uint32_t page_num){
	uint32_t current_page = virtual_mem->free_page;
	uint32_t* header;
	for(int page = 0; page < virtual_mem->num_free_pages; page++){
	    if(current_page == page_num){
	        return true;
	    }
	    header = (uint32_t*)((char*)virtual_mem->start_addr + (size_t)(current_page*BYTES_PER_PG));
	    current_page = *header;
	}
	return false;	
}

static bool is_valid_mapping(vm_t* virtual_mem, paddr_t pt, vaddr_t addr){
//check that pt, pt2, alloc_page is not in free_list
//check that the valid bit is set for pt and pt2
//TODO: do I need to worry about present here?
	uint32_t page_num = pt/BYTES_PER_PG;
	if(is_in_free_list(virtual_mem, page_num)){
	    return false;
	}
	uint32_t pt1_index = bit_extraction(START_BIT_PT1_INDEX, NUM_BITS_PT_INDEX, addr);
	uint32_t pt2_index = bit_extraction(START_BIT_PT2_INDEX, NUM_BITS_PT_INDEX, addr);
	uint32_t page_offset = bit_extraction(START_BIT_PAGE_OFFSET, NUM_BITS_PAGE_OFFSET, addr);
	uint32_t* pte_1 = pte_extraction(virtual_memory->start_addr, pt, pt1_index);
	if(!check_permissions(VALID, *pte_1)){
	    return false;	
	}
	uint32_t ppn_pt2 = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, pte_1);
	if(is_in_free_list(virtual_mem, ppn_pt2)){
	    return false;
	}
	paddr_t pt2 = BYTES_PER_PG*ppn_pt2;
	uint32_t* pte_2 = pte_extraction(virtual_memory->start_addr, pt2, pt2_index);
	if(!check_permissions(VALID, *pte_2)){
	    return false;
	}
	uint32_t ppn_phys_mem = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, pte_2);
	if(is_in_free_list(virtual_mem, ppn_phys_mem)){
	    return false;
	}
        return true;
}

static void overwrite_bit_range(uint32_t start_bit_inclusive, uint32_t end_bit_exclusive, uint32_t* entry, uint32_t num_to_write){
    uint32_t mask = ~(((1 << end_bit_exclusive) - 1)& (~((1 << start_bit_inclusive) - 1)));
    //populates the bit range with zeros, so that we can just bitwise OR our number into the entry
    *entry &= mask;
    //writing our number into the entry
    *entry |= (num_to_write << start_bit_inclusive);
    return;
}

void *vm_init(void *phys_mem, size_t num_phys_pages, FILE *swap, size_t num_swap_pages) {
	vm_t* virtual_mem = (vm_t *)phys_mem;
	virtual_mem->start_addr = (void *)phys_mem; 
	virtual_mem->num_pages = num_phys_pages;
	virtual_mem->num_active_processes = 0;
	virtual_mem->num_free_pages = num_phys_pages - 1;
	virtual_mem->free_page = 1;
	for(int page = 1; page < num_phys_pages; page++){
	    set_free_page_header(page, page + 1, virtual_mem);	
	}
    	return (void *) phys_mem;
}



void vm_deinit(void *vm) {
    // TODO
}

vm_result_t vm_map_page(void *vm, bool new_process, paddr_t pt, vaddr_t addr,
                        bool user, bool exec, bool write, bool read) {
	
	vm_t* virtual_mem = (vm_t*)vm;
	if(is_valid_mapping(virtual_mem, pt, addr)){
	    vm_result_t result = {VM_DUPLICATE, 0};
	    return result;
	}
	//first check if the mapping is already valid, next check if there are even sufficient free pages
	if(new_process){
	    if(virtual_mem->num_free_pages < NUM_FREE_PAGES_NEW_PROCESS){
	    	vm_result_t result = {VM_OUT_OF_MEM, 0};
	    	return result;
	    }
	    //deal with using up the 3 pages now and changing the appropriate fields in the vm_t struct to reflect that
	    uint32_t pt1_index = bit_extraction(START_BIT_PT1_INDEX, NUM_BITS_PT_INDEX, addr);
	    uint32_t pt2_index = bit_extraction(START_BIT_PT2_INDEX, NUM_BITS_PT_INDEX, addr);
	    uint32_t page_offset = bit_extraction(START_BIT_PAGE_OFFSET, NUM_BITS_PAGE_OFFSET, addr);
	    uint32_t pt1_ppn = virtual_mem->free_page;
	    void* pt1 = (uint32_t*)((char*)virtual_mem->start_addr + (size_t)(pt1_ppn*BYTES_PER_PG));
	    uint32_t pt2_ppn = *(uint32_t*)pt1;
	    void* pt2 = (uint32_t*)((char*)virtual_mem->start_addr + (size_t)(pt2_ppn*BYTES_PER_PG));
	    uint32_t mem_page_ppn = *(uint32_t*)pt2;
	    void* mem_page = (uint32_t*)((char*)virtual_mem->start_addr + (size_t)(mem_page_ppn*BYTES_PER_PG));
	    virtual_mem->free_page = *(uint32_t*)mem_page;
	    virtual_mem->num_free_pages -= NUM_FREE_PAGES_NEW_PROCESS;
	    uint32_t* pte1 = (uint32_t*)((char*)pt1 + (size_t)pt1_index);
	    uint32_t* pte2 = (uint32_t*)((char*)pt2 + (size_t)pt2_index);
	    overwrite_bit_range(START_BIT_PPN, END_BIT_PPN_EXCLUSIVE, pte1, pt2_ppn);
	    overwrite_bit_range(START_BIT_PPN, END_BIT_PPN_EXCLUSIVE, pte2, mem_page_ppn);
	    if(user){
	    
	    }
	    if(exec){}
	    if(read){}
	    if(write){}

	    
	}
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
	uint32_t* pte_1 = pte_extraction(virtual_memory->start_addr, pt, pt1_index);
	if(check_permissions(VALID, *pte_1)&&check_permissions(PRESENT, *pte_1)){
		uint32_t ppn_pt2 = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, *pte_1);
		paddr_t pt2 = BYTES_PER_PG*ppn_pt2;
		uint32_t* pte_2 = pte_extraction(virtual_memory->start_addr, pt2, pt2_index);
		//TODO: get rid of all this bool nonsense
		bool inaddequate_permissions = false;
		if(check_permissions(VALID, *pte_2)&&check_permissions(PRESENT, *pte_2)){
	    	    if(!check_permissions(ACCESSED, *pte_1)){
	    		set_perm_bit(virtual_memory->start_addr, pt2, pt2_index, ACCESSED);
	    	    }		
	    	    if(!check_permissions(ACCESSED, *pte_2)){
	    		set_perm_bit(virtual_memory->start_addr, pt2, pt2_index, ACCESSED);
	    	    }		
	    	    if(user){
	    	        if(!check_permissions(USER, pte_2)){
		    	    vm_result_t result = {VM_BAD_PERM, 0};
		    	    return result; 
		        }
	            }
	    	    switch (acc){
	    	        case VM_EXEC: 
	    		    if(!check_permissions(EXECUTABLE, pte_2)){
		    	        vm_result_t result = {VM_BAD_PERM, 0};
		    	        return result; 
			    } 
			    break;      
	                case VM_READ: 
	    		    if(!check_permissions(READABLE, pte_2)){
		    	        vm_result_t result = {VM_BAD_PERM, 0};
		    	        return result; 
			    } 
			    break;      
			      
	                case VM_WRITE: 
	       		    if(!check_permissions(WRITABLE, pte_2)){
		    	        vm_result_t result = {VM_BAD_PERM, 0};
		    	        return result; 
			    } 
	 		    break;    
	        }
	    }else{
		vm_result_t result = {VM_BAD_ADDR, 0};
		return result;
	    }
	    uint32_t ppn = bit_extraction(START_BIT_PPN, NUM_BITS_PPN, pte_2);
	    paddr_t result_addr = BYTES_PER_PG*ppn;
	    vm_result_t result = {VM_OK, result_addr};
            return result;
	}else{
	    vm_result_t result = {VM_BAD_PERM, 0};
	    return result;     
	}
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

