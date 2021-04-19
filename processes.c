#include <stdio.h>
#include <stdlib.h>
#include "processes.h"
#include "pagetable.h"

void initialize_pcb (FILE* fptr, PCB* pcb_ptr, int num_processes) {
    int i = 0; 
    char *proc_input_file;
       
    for (i = 0 ; i < num_processes ; i++) {
        pcb_ptr[i].pid = i;                                                // assign a pid to the process
        pcb_ptr[i].process_state = READY;                                  // initializing all process states to READY
        pcb_ptr[i].num_traces_context_sw = (rand() % 100) + 200;           // assign a random number of traces before context switch (200-300)
        
        fscanf(fptr,"%s\n",pcb_ptr[i].filename);                           // get input file name for each process
        pcb_ptr[i].proc_input_file = fopen (pcb_ptr[i].filename, "r");     // get input file stream pointer for each process
        
        if (pcb_ptr[i].proc_input_file == NULL) {
            printf(" ERROR: Could not open the input file for process %d\n", pcb_ptr[i].pid); 
        } 
        
        // page directory base address
        pcb_ptr[i].page_dir_base_addr = page_dir_init();

    }
}

void initialize_access_info_structs (Proc_Access_Info* proc_access_info, int num_processes) {
    int i = 0; 
       
    for (i = 0 ; i < num_processes ; i++) {
    
        proc_access_info[i].pid = i;
     
        proc_access_info[i].num_l1_tlb_accesses = 0;
        proc_access_info[i].num_l1_tlb_hits = 0;
        proc_access_info[i].num_l1_tlb_misses = 0;
   
        proc_access_info[i].num_l2_tlb_accesses = 0;
        proc_access_info[i].num_l2_tlb_hits = 0;
        proc_access_info[i].num_l2_tlb_misses = 0;
    
        proc_access_info[i].num_l1_cache_accesses = 0;
        proc_access_info[i].num_l1_cache_hits = 0;
        proc_access_info[i].num_l1_cache_misses = 0; 
        proc_access_info[i].num_l1_cache_predetermined_misses = 0;
        // proc_access_info[i].num_l1_cache_av_ways_opened = 0.0; 
        
        proc_access_info[i].num_l2_cache_accesses = 0;
        proc_access_info[i].num_l2_cache_hits = 0;
        proc_access_info[i].num_l2_cache_misses = 0;

        proc_access_info[i].num_main_memory_accesses = 0;
        proc_access_info[i].num_main_memory_hits = 0;
        proc_access_info[i].num_main_memory_misses = 0; // page faults 
    
        proc_access_info[i].page_fault_frequency = 0.0;
    }
}

void prepaging_function (PCB* pcb_ptr, int num_processes) {

    int i = 0;
    unsigned int logical_address_page1;
    unsigned int logical_address_page2;

    // For each READY process  
    for (i = 0 ; i < num_processes ; i++) {
        if (pcb_ptr[i].process_state = READY) {
            
            fscanf(pcb_ptr[i].proc_input_file, "%x", &logical_address_page1);
            printf(" Process %d logical address 1: %x\n", i, logical_address_page1);
            
            // TODO: Pre-page page corresponding to logical_address_page1 
            get_page_entry((logical_address_page1 >> 9), pcb_ptr/*, proc_access_info*/); // page table walk
            
            // Keep reading next traces till next request is from another (distinct) page
            while (1) {
                 fscanf(pcb_ptr[i].proc_input_file, "%x", &logical_address_page2);
                 if ((logical_address_page2 >> 9) != (logical_address_page1 >> 9))
                     break;
            }
            
            printf(" Process %d logical address 2: %x\n", i, logical_address_page2);
            printf("\n");
            
            // TODO: Pre-page page corresponding to logical_address_page2 
            get_page_entry((logical_address_page2 >> 9), pcb_ptr/*, proc_access_info*/); // page table walk
            
            // Reset file stream pointers to start --TODO fseek(fptr, 0, SEEK_SET); rewind(fptr);
            // fclose(pcb_ptr[i].proc_input_file);
            // pcb_ptr[i].proc_input_file = fopen (pcb_ptr[i].filename, "r");
            fseek(pcb_ptr[i].proc_input_file, 0, SEEK_SET);
        }
    }
}

void print_pcb (PCB* pcb_ptr, int num_processes) {
    int i = 0;
    
    for (i = 0 ; i < num_processes ; i++) {
        printf(" Process ID: %d\n", pcb_ptr[i].pid);
        printf(" Process State: %d\n", pcb_ptr[i].process_state);
        printf(" Max number of traces simulated before context switch: %d\n", pcb_ptr[i].num_traces_context_sw);
        printf(" File name: %s\n", pcb_ptr[i].filename);
        printf(" Page Directory Base Addr: %x\n", pcb_ptr[i].page_dir_base_addr); 
        printf("\n");
    }
   
}



























