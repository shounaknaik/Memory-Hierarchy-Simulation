// print pcb
// resolve tlb errors


#include <stdio.h>
#include <stdlib.h>
#include <time.h>     // for srand 
#include "l1_cache.h"
#include "tlb.h"
#include "processes.h"

int main (int argc, char *argv[]) {

    int num_processes;                          // Total number of processes
    FILE *fptr;                                 // Input file pointer
    PCB *pcb_ptr;                               // Pointer to the pcb structure array
    
    trace_info trace;                           // Trace info struct - consists of address bit fields
    
    unsigned int frame_number_returned = 0;     // Temporary variable to store return value of TLB search functions
    
    L1_cache *l1_cache_access_ptr;              // To set l1 access pointer to l1 instruction/data cache pointer depending on trace type
    int l1_cache_access_type = 0;               // To set l1 cache access depending on trace type
    data_byte l1_cache_write_data;              // Random data value chosen for l1 cache write simulation -- in an actual system, the processor will write the result obtained to the data memory  
    l1_cache_write_data.data = 255;
    unsigned int l1_data_returned = 0;          // Temporary variable to store return value of L1 cache search function    
    
    int shared_bit;          
              
    // To maintain TLB access stats
   
    int num_l1_tlb_accesses = 0;
    int num_l1_tlb_hits = 0;
    int num_l1_tlb_misses = 0;
   
    int num_l2_tlb_accesses = 0;
    int num_l2_tlb_hits = 0;
    int num_l2_tlb_misses = 0;    
    int num_tlb_page_faults = 0;
   
    double page_fault_fequency = 0.0;
    
    // To maintain L1 cache access stats
    
    int num_l1_instr_cache_accesses = 0;
    int num_l1_instr_cache_hits = 0;
    int num_l1_instr_cache_misses = 0; 
    int num_l1_instr_cache_predetermined_misses = 0; 
    
    int num_l1_data_cache_write_accesses = 0;
    int num_l1_data_cache_read_accesses = 0;
    int num_l1_data_cache_hits = 0;
    int num_l1_data_cache_misses = 0;
    // int num_l1_data_cache_predetermined_misses = 0;
    
    // Initialize all memory subsystem structures

    // Initialize L1 TLB
    L1_TLB *l1_tlb;
    l1_tlb = initialize_L1_TLB ();
    
    // Initialize L2 TLB
    L2_TLB *l2_tlb;
    l2_tlb = initialize_L2_TLB ();       
    
    // Initialize L1 INSTRUCTION cache
    L1_cache *l1_instr_cache;
    l1_instr_cache = initialize_L1_cache (INSTRUCTION);
    
    // Initialize L1 DATA cache
    L1_cache *l1_data_cache;
    l1_data_cache = initialize_L1_cache (DATA);          

    // Open and read input file
    fptr = fopen ("process_files.txt","r");
    if (fptr == NULL) {
        printf(" ERROR: Could not open the input file\n");
        return -1; 
    } 
    
    // Read number of processes and allocate memory for PCB struct array
    fscanf (fptr,"%d\n", &num_processes);
    pcb_ptr = malloc (num_processes * sizeof (PCB));

    // Initialize the PCB structures for all processes
    srand(time(0));  // seeding rand with time(0) to generate random numbers 
    initialize_pcb (fptr, pcb_ptr, num_processes);
    print_pcb (pcb_ptr, num_processes);

    // Close input file   
    fclose (fptr);
    
    // Loading the READY processes by prepaging the first 2 pages
    prepaging_function (pcb_ptr, num_processes);
    
    int i = 0; 
    int j = 0;
  
    // Begin simulation... execute all processes sequentially (with context switches) to completion
    for (i = 0; i < num_processes; i++) {
    
        printf (" Now simulating PROCESS %d traces ...\n\n", i);
     
        // Till number of traces simulated reaches max limit before context switch
        // Carry out simulations for process i    
        for (j = 0; j < pcb_ptr[i].num_traces_context_sw; j++) {
        
            // Read next trace (32-bit logical address) from input file
            unsigned int temp = 0;
            fscanf(pcb_ptr[i].proc_input_file, "%x", &temp);                                                         // check return value for EOF?
            trace.logical_address = temp;
            printf(" Logical address (trace): %x\n", trace.logical_address);           
            
            // Determining if the trace belongs to instruction series (7f) or data series (10) 
            // - reqd to determine L1 (split) cache type
            if ((trace.logical_address >> 24) == 0x7f)
                trace.trace_type = 0;//INSTRUCTION;
            else
                trace.trace_type = 1;//DATA;
            
            // Extract 23-bit page number from the 32-bit logical address
            trace.page_number = trace.logical_address >> 9;
            printf(" Initiating TLB search for entries with the corresponding page number: %x\n", trace.page_number);
            
            // ------------------------------------- TLB search ----------------------------------------------
                
            // Search L1 TLB 
            frame_number_returned = search_L1_TLB (l1_tlb, trace.page_number);
            num_l1_tlb_accesses++;

            // If the returned frame number is within valid range - L1 TLB HIT - frame number acquired
            if (frame_number_returned >= 0 && frame_number_returned <= MAX_FRAME_NUMBER) {
                trace.frame_number = frame_number_returned;
                printf(" L1 TLB hit!\n Entry found - Page number %x, Frame number %x\n", trace.page_number, trace.frame_number);
                num_l1_tlb_hits++;          
            }
        
            // If the returned frame number is NOT within valid range - L1 TLB MISS
            else { 
                printf(" L1 TLB miss!\n");
                num_l1_tlb_misses++;
            
                // Search L2 TLB
                frame_number_returned = search_L2_TLB (l2_tlb, trace.page_number);
                num_l2_tlb_accesses++;
            
                // If the returned frame number is within valid range - L2 TLB HIT - frame number acquired
                if (frame_number_returned >= 0 && frame_number_returned <= MAX_FRAME_NUMBER) {
                    trace.frame_number = frame_number_returned;
                    printf(" L2 TLB hit!\n Entry found - Page number %x, Frame number %x\n", trace.page_number, trace.frame_number);
                    num_l2_tlb_hits++;
                }
                
                // If the returned frame number is NOT within valid range - L2 TLB MISS
                else {
                    printf(" L2 TLB miss!\n");
                    num_l2_tlb_misses++;
            
                    // EXCEPTION: Kernel performs page table walk and updates the TLB entry
                    // in both the levels (L2 TLB as well as L1 TLB)
                    // TODO: frame_number_returned = get_mm_block();
                
                
                    // Update L2 TLB with the acquired entry -- KERNEL
                    update_L2_TLB (l2_tlb, trace.page_number, frame_number_returned, shared_bit);
                    printf(" L2 TLB updated by Kernel!\n");
                    // print_L2_tlb (l2_tlb);
                
                    // Update L1 TLB with the acquired entry -- KERNEL
                    update_L1_TLB (l1_tlb, trace.page_number, frame_number_returned, shared_bit);
                    printf(" L1 TLB updated by Kernel!\n");
                    // print_L1_tlb (l1_tlb);
                    
                    // Accessing L1 TLB again, would result DEFINITELY result in a hit
                    // Search L1 TLB 
                    frame_number_returned = search_L1_TLB (l1_tlb, trace.page_number);
                    num_l1_tlb_accesses++;

                    // If the returned frame number is within valid range - L1 TLB HIT - frame number acquired
                    if (frame_number_returned >= 0 && frame_number_returned <= MAX_FRAME_NUMBER) {
                        trace.frame_number = frame_number_returned;
                        printf(" L1 TLB hit after updation!\n Entry found - Page number %x, Frame number %x\n", trace.page_number, trace.frame_number);
                        num_l1_tlb_hits++;          
                    }
                }
            }
            
            // Getting physical address from the frame number 
            trace.physical_address = ((trace.frame_number << 9) | (trace.logical_address % 512));
            printf (" Therefore, corresponding physical address: %x\n\n", trace.physical_address);
            
            // -------------------------------------------------- Cache ops -------------------------------------------------------
            
            printf(" Initiating L1 cache search for datablock entries corresponding to the physical address %x\n", trace.physical_address);
            
            // Determine which L1 cache (INSTRUCTION or DATA) is to be accessed depending on trace type
            
            // If the trace is instruction address - L1 instruction cache to be accessed, access type is always read
            if (trace.trace_type == INSTRUCTION) { 
                 l1_cache_access_ptr = l1_instr_cache;
                 l1_cache_access_type = READ_ACCESS;
                 num_l1_instr_cache_accesses++;
            }
            
            // If the trace is data address - L1 data cache to be accessed, access type approximately 72% read 28% write (P&H) 
            else {
                 l1_cache_access_ptr = l1_data_cache;
                 num_l1_data_cache_accesses++;
            
                 // To simulate different access types - generate a random number that gives a 0 -
                 // - read 75% (e*o, e*e, o*e all div by 2) times, 1 - write 25% - (o*o not div by 2) times
                 if ((rand() % 2) * (rand() % 2)) {
                     l1_cache_access_type = READ_ACCESS;
                     num_l1_data_cache_read_accesses++;
                 }
                 else {
                     l1_cache_access_type = WRITE_ACCESS;
                     num_l1_data_cache_write_accesses++;
                 }
            }
            
            // Search entry corresponding to the given physical address in L1 cache - if found, read/write the corresponding - 
            // - datablock depending on the access type - if NOT found LOOK- THROUGH to L2 cache and update L1 with the datablock entry reqd
            l1_data_returned = search_L1_cache (l1_cache_access_ptr, trace.physical_address, l1_cache_write_data.data, l1_cache_access_type);
                        
            if (l1_data_returned >= 0 && l1_data_returned <= 255) {
                if (trace.trace_type == INSTRUCTION)
                    printf(" L1 INSTRUCTION cache hit!\n The processor successfully read the data %x corresponding to the address %x from L1 INSTRUCTION cache.\n", l1_data_returned, trace.physical_address);
                else
                    printf(" L1 DATA cache hit!\n The processor successfully read the data from the datablock %x corresponding to the address %x from L1 DATA cache.\n", l1_data_returned, trace.physical_address);
            }
            
            else if (l1_data_returned == L1_CACHE_WRITE_SUCCESSFUL) {
                printf(" L1 DATA cache hit!\n The processor successfully wrote the data into the datablock %x corresponding to the address %x into L1 DATA cache.\n", l1_data_returned, trace.physical_address);
            }
            
            else if (l1_data_returned == L1_CACHE_WRITE_PROTECTION_EXCEPTION) {
                printf(" L1 DATA cache hit/miss(?)\n The datablock %x corresponding to the address %x was found in L1 DATA cache.\n But the process executing does not have the permission to write into this datablock.\n Execute WRITE PROTECT EXCEPTION routine for process %d...\n", l1_data_returned, trace.physical_address, i); 
            }
            
            else {
                printf(" L1 cache miss!\n");
                
                // If the entry corresponding to the given physical address is not found in L1, search in L2 (L1 follows look-through policy) 
                // L2 search called from update_L1_cache function
                
                // But since L2 is look-aside, when L2 search is initiated, L2 sends a signal to start searching main memory
                // Main memory search called from update_L2_cache function
                
                // ( Since, in reality, we would perform these 2 searches parallel-y, if L2 is a hit Main Memory lite - accept L2 return value
                // Else accept Main Memory return value ) 
                
            }           
        }
        
        pcb_ptr[i].process_state = READY;
        printf(" Context Switch! Flushing the TLB (retaining all shared entries)...\n\n");
        flush_L1_TLB (l1_tlb);
        flush_L2_TLB (l2_tlb);            
    }
    

    // Close files on process termination
    
    return 0;
}
