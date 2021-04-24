#ifndef PROCESSES_H
#define PROCESSES_H

#include <stdio.h>
#include "pagetable.h"

// Process states -- according to the 3-state diagram

#define READY 1        // Process is ready for OS to assign it to the CPU 
#define RUNNING 2      // CPU is executing process’s instructions
#define WAITING 3      // When process is not READY/RUNNING process is waiting for some requested service
#define TERMINATED 4   // Process completed execution

#define MAX_PAGE_FAULT_FREQUENCY 0.01 // 1%
#define MIN_PAGE_FAULT_FREQUENCY 0.0000025 // 0.00025%

// PCB structure maintained per process

typedef struct {
    int pid;	                       // Unique integer assigned by OS when process is created
    int process_state;                 // READY, RUNNING or WAITING
    int num_traces_context_sw;         // Maximum number of requests serviced before context switch
    char filename[100];                // Input file name containing traces from that process
    FILE *proc_input_file;             // Input file pointer containing traces from that process
    // pointer to page table           // Pointer to page table structure of that process (mem context info)
    page_table* page_dir_base_addr;
    unsigned int page_count;           // Total number of pages held by this process
} PCB;

typedef struct {
    unsigned int logical_address:32;   // Each trace in input file corresponds to the logical addresses of instructions executed and data referenced during a program's execution
    unsigned int physical_address:25;  // Physical address corresponding to the trace (requested address) 
    unsigned int page_number:23;       // Page number corresponding to the trace (requested address)
    unsigned int frame_number:16;      // Frame number corresponding to the trace (requested address) 
    int trace_type;                    // To identify whether the trace belongs to instruction series (7f...) or data series (10...) 
} trace_info;


// To maintain L1 cache access stats

typedef struct {
    int pid;
     
    // TLB L1 access info
    int num_l1_tlb_accesses;
    int num_l1_tlb_hits;
    int num_l1_tlb_misses;
   
    // TLB L2 access info
    int num_l2_tlb_accesses;
    int num_l2_tlb_hits;
    int num_l2_tlb_misses;

    // L1 cache access info
    int num_l1_cache_accesses;
    int num_l1_cache_hits;
    int num_l1_cache_misses; 
    int num_l1_cache_predetermined_misses;
    
    // double num_l1_cache_av_ways_opened; -- maintained as global variable in l1_cache_functions.c - no point maintaining per process
    // maintained for overall system 
        
    // L2 cache access info
    int num_l2_cache_accesses;
    int num_l2_cache_hits;
    int num_l2_cache_misses;

    // Main memory access info
    int num_main_memory_accesses;
    int num_main_memory_hits;
    int num_main_memory_misses; // page faults 
    
    double page_fault_frequency;
    
} Proc_Access_Info;

void initialize_pcb (FILE* fptr, PCB* pcb_array, int num_processes);                                              // Initializes the Process Control Block fields for all processes
void prepaging_function (PCB* pcb_array, Proc_Access_Info* proc_access_info, int num_processes);                  // Loads the first 2 pages of all READY processes
void print_pcb (PCB* pcb_array, int num_processes);
void initialize_access_info_structs (Proc_Access_Info* proc_access_info, int num_processes);

page_table* page_dir_init();
page_table_entry *get_page_entry(unsigned int block_number /*virtual address*/, PCB* temp_pcb, Proc_Access_Info* temp_pai);
void invalidate_page(unsigned int p_table_index);
void page_table_free(page_table* p_table);

#endif
