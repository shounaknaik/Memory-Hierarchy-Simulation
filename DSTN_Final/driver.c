// TODO: change all headers -- done
// TODO: pass proc_access_info pointer to main memory functions -- modify main memory functions accordingly

#include <stdio.h>
#include <stdlib.h>
#include <time.h>     // for srand 

#include "tlb.h"
#include "cache.h"

#include "pagetable.h"
#include "processes.h"
#include "mainmemory.h"

int main (int argc, char *argv[]) {

   int num_processes;                          // Total number of processes
   PCB *pcb_ptr;                               // Pointer to the pcb structure array
    
   FILE *fptr;                                 // Input file pointer
    
   trace_info trace;                           // Trace info struct - consists of address bit fields
    
   unsigned int frame_number_returned = 0;     // Temporary variable to store return value of TLB search functions
   page_table_entry* pte;                      // Temporary variable to hold return value of page table walk function
   
   page_table *temp_pt;
    
   L1_cache *l1_cache_access_ptr;              // To set l1 access pointer to l1 instruction/data cache pointer depending on trace type
   int l1_cache_access_type = 0;               // To set l1 cache access depending on trace type
    
   unsigned int l1_data_returned = 0;          // Temporary variable to store return value of L1 cache search function         
   data_byte l1_cache_write_data;              // Temporary variable to data to be written into L1 cache
   l1_cache_write_data.data = 255;             // Random data value chosen for l1 cache write simulation -- in an actual system, the processor will write the result obtained to the data memory 
   
   // Pointer to structure to store L1 datablock returned by L2 cache search function after miss in L1 cache
   data_byte *l2_l1_data_block_returned;  
   l2_l1_data_block_returned = malloc (NUM_L1_CACHE_BLOCK_SIZE * sizeof (data_byte));
   
   // Pointer to structure to store L2 datablock returned by main memory search function after miss in L1 cache
   data_byte *mm_l2_data_block_returned;  
   mm_l2_data_block_returned = malloc (NUM_L2_CACHE_BLOCK_SIZE * sizeof (data_byte));
    
   double tlb_L1_hit_rate = 0.0;
   double tlb_L2_hit_rate = 0.0;
   double L1_cache_hit_rate = 0.0;
   double L2_cache_hit_rate = 0.0;
   // double main_memory_hit_rate = 0.0;
   double page_fault_frequency = 0.0;          
   
   // Initialize all the memory subsystem structures
    
   main_memory *mm_ptr;
   mm_ptr = main_memory_init();
     
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
    
   // Initialize L2 cache
   L2_cache *l2_cache;                 
   l2_cache = initialize_L2_cache ();        

   // Open and read input file
   fptr = fopen ("process_files.txt","r");
   if (fptr == NULL) {
       // printf(" ERROR: Could not open the input file\n");
       return -1; 
   } 
    
   // Read number of processes from input file and allocate memory for PCB struct array
   fscanf (fptr,"%d\n", &num_processes);
   pcb_ptr = malloc (num_processes * sizeof (PCB));

   // Initialize the PCB structures for all processes
   srand(time(0));  // seeding rand with time(0) to generate random numbers 
   initialize_pcb (fptr, pcb_ptr, num_processes);
 
   // print_pcb (pcb_ptr, num_processes);

   // Close input file
   fclose (fptr);
   
   // Initialize per process access info struct array
   Proc_Access_Info *proc_access_info;
   proc_access_info = malloc (num_processes * sizeof (Proc_Access_Info));
   initialize_access_info_structs (proc_access_info, num_processes);

   // First 2 blocks of all the READY processes are prepaged
   prepaging_function (pcb_ptr, proc_access_info, num_processes);
    
   int i = 0; 
   int j = 0;
   unsigned int temp = 0; // to scan trace (logical address)
   int shared_bit = 0;
   
   int fscanf_retval = 0; 
  
   while (1) {
   
       for (i = 0; i < num_processes; i++) {
    
           if (pcb_ptr[i].process_state == READY) {
               // printf (" Now simulating PROCESS %d traces ...\n\n", i);
     
               // Till number of traces simulated reaches max limit before context switch
               // Carry out simulations for process i 
                  
               for (j = 0; j < pcb_ptr[i].num_traces_context_sw; j++) {
        
                   // Read next trace (32-bit logical address) from input file
                   fscanf_retval = fscanf(pcb_ptr[i].proc_input_file, "%x", &temp);
               
                   // If EOF detected, TERMINATE the process, flush TLBs and free the memory structs
                   if (fscanf_retval == EOF) {
                       pcb_ptr[i].process_state = TERMINATED;
                       fclose(pcb_ptr[i].proc_input_file);
                  
                       // TODO: free page table, invalidate frames
                       page_table_free(pcb_ptr[i].page_dir_base_addr);
                  
                       flush_L1_TLB (l1_tlb);
                       flush_L2_TLB (l2_tlb);
                       break;
                   }
               
                   trace.logical_address = temp;
                   // printf(" Logical address (trace): %x\n", trace.logical_address);           
           
                   // Determining if the trace belongs to instruction series (7f) or data series (10) 
                   // - reqd to determine L1 (split) cache type
                   if ((trace.logical_address >> 24) == 0x7f)
                       trace.trace_type = INSTRUCTION;
                   else
                       trace.trace_type = DATA;
            
                   // Extract 23-bit page number from the 32-bit logical address
                   trace.page_number = trace.logical_address >> 9;
                   // printf(" Initiating TLB search for entries with the corresponding page number: %x\n", trace.page_number);
             
                   // ------------------------------------- TLB search ----------------------------------------------
                
                   // Search L1 TLB 
                   frame_number_returned = search_L1_TLB (l1_tlb, trace.page_number);
                   proc_access_info[i].num_l1_tlb_accesses++;

                   // If the returned frame number is within valid range - L1 TLB HIT - frame number acquired
                   if (frame_number_returned >= 0 && frame_number_returned <= MAX_FRAME_NUMBER) {
                       trace.frame_number = frame_number_returned;
                       // printf(" L1 TLB hit!\n Entry found - Page number %x, Frame number %x\n", trace.page_number, trace.frame_number);
                       proc_access_info[i].num_l1_tlb_hits++;          
                   }
        
                   // If the returned frame number is NOT within valid range - L1 TLB MISS
                   else { 
                       // printf(" L1 TLB miss!\n");
                       proc_access_info[i].num_l1_tlb_misses++;
           
                       // Search L2 TLB
                       frame_number_returned = search_L2_TLB (l2_tlb, trace.page_number);
                       proc_access_info[i].num_l2_tlb_accesses++;
           
                       // If the returned frame number is within valid range - L2 TLB HIT - frame number acquired
                       if (frame_number_returned >= 0 && frame_number_returned <= MAX_FRAME_NUMBER) {
                           trace.frame_number = frame_number_returned;
                           // printf(" L2 TLB hit!\n Entry found - Page number %x, Frame number %x\n", trace.page_number, trace.frame_number);
                           proc_access_info[i].num_l2_tlb_hits++;
                           
                           // L1 TLB updation after L2 TLB hit
                           update_L1_TLB (l1_tlb, trace.page_number, frame_number_returned, shared_bit);
                       }
                
                       // If the returned frame number is NOT within valid range - L2 TLB MISS
                       else {
                           // printf(" L2 TLB miss!\n");
                           proc_access_info[i].num_l2_tlb_misses++;
           
                           // EXCEPTION: Kernel performs page table walk and updates the TLB entry in both the levels (first L2 TLB, then L1 TLB)
                           pte = get_page_entry (trace.page_number, pcb_ptr, proc_access_info);
                           proc_access_info[i].num_main_memory_accesses++;
                           frame_number_returned = pte->pageframe.frame_num;
                           shared_bit = pte->shared_bit;
                       
                           // Number of main memory hits/misses updated in main memory functions - we check the updated value of page fault frequency
                           proc_access_info[i].page_fault_frequency = (double)proc_access_info[i].num_main_memory_misses/(double)proc_access_info[i].num_main_memory_accesses;

                           // Check for thrashing after every main memory access 
                           /*                           
                           if (proc_access_info[i].page_fault_frequency > MAX_PAGE_FAULT_FREQUENCY) {
                               // printf(" Process page fault frequency exceeds MAX LIMIT..THRASHING DETECTED!\n Swapping process out by changing state to WAITING\n");
                               pcb_ptr[i].process_state = WAITING;
                               flush_L1_TLB (l1_tlb);
                               flush_L2_TLB (l2_tlb);
                           
                               // Invalidate all of its pages  --- loop (4 times) starting from outermost structure
                               for (int k = 0; k < 4; k++)
                                   invalidate_page(pcb_ptr[i].page_dir_base_addr->entry_table[k].pageframe.p_table_index); // ideally should be swapped into swap space
                               break; 
                           } */                     
                
                           // Update L2 TLB with the acquired entry -- KERNEL
                           update_L2_TLB (l2_tlb, trace.page_number, frame_number_returned, shared_bit);
                           // printf(" L2 TLB updated by Kernel!\n");
                           // print_L2_tlb (l2_tlb);
               
                           // Update L1 TLB with the acquired entry -- KERNEL
                           update_L1_TLB (l1_tlb, trace.page_number, frame_number_returned, shared_bit);
                           // printf(" L1 TLB updated by Kernel!\n");
                           // print_L1_tlb (l1_tlb);
                   
                           // Accessing L1 TLB again, would result DEFINITELY result in a hit - Search L1 TLB 
                           frame_number_returned = search_L1_TLB (l1_tlb, trace.page_number);
                           proc_access_info[i].num_l1_tlb_accesses++;

                           // If the returned frame number is within valid range - L1 TLB HIT - frame number acquired
                           if (frame_number_returned >= 0 && frame_number_returned <= MAX_FRAME_NUMBER) {
                               trace.frame_number = frame_number_returned;
                               // printf(" L1 TLB hit after updation!\n Entry found - Page number %x, Frame number %x\n", trace.page_number, trace.frame_number);
                               proc_access_info[i].num_l1_tlb_hits++;          
                           }
                       }
                   }
                      
                   // Getting physical address from the frame number 
                   trace.physical_address = ((trace.frame_number << 9) | (trace.logical_address % 512));
                   // printf (" Therefore, corresponding physical address: %x\n\n", trace.physical_address);
           
                   // -------------------------------------------Cache & Memory accesses -------------------------------------------------------
            
                   // printf(" Initiating L1 cache search for datablock entries corresponding to the physical address %x\n", trace.physical_address);
            
                   // Determine which L1 cache (INSTRUCTION or DATA) is to be accessed depending on trace type
            
                   // If the trace is instruction address - L1 instruction cache to be accessed, access type is always read
                   if (trace.trace_type == INSTRUCTION) { 
                       l1_cache_access_ptr = l1_instr_cache;
                       l1_cache_access_type = READ_ACCESS;
                       // num_l1_instr_cache_accesses++;
                   }
            
                   // If the trace is data address - L1 data cache to be accessed, access type approximately 72% read 28% write (P&H) 
                   else {
                       l1_cache_access_ptr = l1_data_cache;
                       // num_l1_data_cache_accesses++;
                   
                       // To simulate different access types - generate a random number that gives a 0 -
                       // - read 75% (e*o, e*e, o*e all div by 2) times, 1 - write 25% - (o*o not div by 2) times
                       if ((rand() % 2) * (rand() % 2)) {
                           l1_cache_access_type = WRITE_ACCESS;
                           // num_l1_data_cache_write_accesses++;
                       }
                
                       else {
                           l1_cache_access_type = READ_ACCESS;
                           // num_l1_data_cache_read_accesses++;
                       }
                   }
            
                   // Search entry corresponding to the given physical address in L1 cache - if found, read/write the corresponding - 
                   // - datablock depending on the access type 
                   l1_data_returned = search_L1_cache (l1_cache_access_ptr, trace.physical_address, l1_cache_write_data.data, l1_cache_access_type);
                   proc_access_info[i].num_l1_cache_accesses++;
                   
                   // If data returned by L1 cache is within valid range, L1 cache HIT for READ access     
                   if (l1_data_returned >= 0 && l1_data_returned <= 255) {
                       proc_access_info[i].num_l1_cache_hits++;
                   
                       if (trace.trace_type == INSTRUCTION) {
                           // num_l1_instr_cache_hits++;
                           // printf(" L1 INSTRUCTION cache hit!\n The processor successfully read the data %x corresponding to the address %x from L1 INSTRUCTION cache.\n", l1_data_returned, trace.physical_address);
                       }
                       else {
                           // num_l1_data_cache_hits++;
                           // printf(" L1 DATA cache hit!\n The processor successfully read the data from the datablock %x corresponding to the address %x from L1 DATA cache.\n", l1_data_returned, trace.physical_address);   
                       }
                   }
               
                   // If the function returns L1_CACHE_WRITE_SUCCESSFUL flag, L1 cache HIT for WRITE access 
                   else if (l1_data_returned == L1_CACHE_WRITE_SUCCESSFUL) {
                       // num_l1_data_cache_hits++;
                       proc_access_info[i].num_l1_cache_hits++;
                       // printf(" L1 DATA cache hit!\n The processor successfully wrote the data into the datablock %x corresponding to the address %x into L1 DATA cache.\n", l1_data_returned, trace.physical_address);
                   }
            
                   // If the function returns L1_CACHE_WRITE_PROTECTION_EXCEPTION flag, L1 cache MISS for WRITE access, since this is an EXCEPTION - Execute exception routine
                   // Second access to L1 will be a HIT in this case 
                   else if (l1_data_returned == L1_CACHE_WRITE_PROTECTION_EXCEPTION) {
                       // printf(" L1 DATA cache miss!\n The datablock %x corresponding to the address %x was found in L1 DATA cache.\n But the process executing does not have the permission to write into this datablock.\n Execute WRITE PROTECT EXCEPTION routine for process %d...\n", l1_data_returned, trace.physical_address, i);
                   
                       // num_l1_data_cache_misses++;
                       proc_access_info[i].num_l1_cache_misses++;
                   
                       // Will execute the exception routine, re-execute same instruction - next time access will result in hit
                   
                       // num_l1_data_cache_hits++; 
                       proc_access_info[i].num_l1_cache_hits++;
                   }
            
                   // if NOT found -- LOOK-THROUGH to L2 cache and update L1 with the datablock entry reqd
                   else {
                       // printf(" L1 cache miss!\n");
                       proc_access_info[i].num_l1_cache_misses++;
                   
                       // *** L2 is Look-aside --> search L2 and main memory simultaneously
                
                       // If the entry corresponding to the given physical address is not found in L1, cache miss stall and search in L2 (L1 follows look-through policy) 
                       l2_l1_data_block_returned = search_L2_cache (l2_cache, trace.physical_address, NULL, READ_ACCESS);
                       proc_access_info[i].num_l2_cache_accesses++;
                       
                       // But since L2 is look-aside, when L2 search is initiated, L2 sends a signal to start searching main memory
                       mm_l2_data_block_returned = get_l2_block (trace.physical_address >> NUM_L2_CACHE_OFFSET_BITS, &proc_access_info[i]);
                
                       // If search L2 cache returns a NULL pointer - L2 cache miss
                       if (l2_l1_data_block_returned == NULL) {
                           // printf(" L2 cache miss!\n");
                           proc_access_info[i].num_l2_cache_misses++;
                           proc_access_info[i].num_main_memory_accesses++;  // incrementing main memory access only when L2 is miss
                                                                            // else, L2 accessed
                       
                           // Number of main memory hits/misses updated in main memory functions - we check the updated value of page fault frequency
                           proc_access_info[i].page_fault_frequency = (double)proc_access_info[i].num_main_memory_misses/(double)proc_access_info[i].num_main_memory_accesses;
                           /*
                           // Check for thrashing after every main memory access                                                  
                           if (proc_access_info[i].page_fault_frequency > MAX_PAGE_FAULT_FREQUENCY) {
                               // printf(" Process page fault frequency exceeds MAX LIMIT..THRASHING DETECTED!\n Swapping process out by changing state to WAITING\n");
                               pcb_ptr[i].process_state = WAITING;
                               flush_L1_TLB (l1_tlb);
                               flush_L2_TLB (l2_tlb);
                           
                               // Invalidate all of its pages  --- loop (4 times) starting from outermost structure
                               for (int k = 0; k < 4; k++)
                                   invalidate_page(pcb_ptr[i].page_dir_base_addr->entry_table[k].pageframe.p_table_index); // ideally should be swapped into swap space
                               break; 
                           } */
                       
                           // Update L2 cache with mm_l2_data_block_returned
                           update_l2_cache (l2_cache, mm_l2_data_block_returned, trace.physical_address);
                           l2_l1_data_block_returned = search_L2_cache (l2_cache, trace.physical_address, NULL, READ_ACCESS);
                       
                           // Update L1 cache with l2_l1_data_block_returned
                           update_L1_cache (l1_cache_access_ptr, l2_cache, l2_l1_data_block_returned, trace.physical_address);                       
                       }
                    
                       // If search L2 cache returns some data - L2 cache hit - sends signal to main memory to call off the search
                       else {
                           // printf(" L2 cache hit!\n");
                           proc_access_info[i].num_l2_cache_hits++;
                           // proc_access_info[i].num_main_memory_hits--; or proc_access_info[i].num_main_memory_misses--; TODO: bug fix HOW TO RECTIFY? 
                           update_L1_cache (l1_cache_access_ptr, l2_cache, l2_l1_data_block_returned, trace.physical_address);
                       }
                   }           
               }
        
               if (j == pcb_ptr[i].num_traces_context_sw) {
                   // Once max number of traces before context switch simulated -- Context switch i.e. the process moves from RUNNING to WAITING/READY
                   // We assume that the process is in READY state by its next turn in process execution sequence - so just changing back the state to READY
                   pcb_ptr[i].process_state = READY; 
        
                   // Context Switch
                   // printf(" Context Switch! Flushing the TLB (retaining all shared entries)...\n\n");
                   flush_L1_TLB (l1_tlb);
                   flush_L2_TLB (l2_tlb);
               }
           }            
       }
       
       // If all processes TERMINATED break out of while loop
       for (i = 0; i < num_processes; i++) {
           if (pcb_ptr[i].process_state == TERMINATED)
               j++;
       }
       
       if (j == num_processes)
           break;  
           
       // THRASHING DETECTION
       page_fault_frequency = 0.0;
              
       // Calculating PFF for all processes (that have not yet completed)
       for (i = 0; i < num_processes; i++) {
            if (pcb_ptr[i].process_state != TERMINATED)
                page_fault_frequency = page_fault_frequency + ((double)proc_access_info[i].num_main_memory_misses/(double)proc_access_info[i].num_main_memory_accesses);
       }
       
       // Swapping out processes
       while (page_fault_frequency > MAX_PAGE_FAULT_FREQUENCY) {
           // printf(" Swap out processes (randomly) by changing their state to WAITING and invalidating all pages (swap out), till active (ready/running) processes PFFs summation is within the set bounds \n");
           i = rand () % num_processes;
           pcb_ptr[i].process_state = WAITING;
           page_fault_frequency = page_fault_frequency - ((double)proc_access_info[i].num_main_memory_misses/(double)proc_access_info[i].num_main_memory_accesses);
                           
           // Invalidate all of its pages  --- loop (4 times) starting from outermost structure
           for (int k = 0; k < 4; k++)
               invalidate_page(temp_pt->entry_table[k].pageframe.p_table_index); // ideally should be swapped into swap space - this code 
                                                                                 // currently does not contain the required ADTs, functions to swap in / swap out pages
       }
   } 
   
   
   // printf("\n\n Memory subsystem simulation complete...\n Printing per process Access Stats ");
   for (i = 0; i < num_processes; i++){
       // printf("\n Printing access info stats for process %d ...\n", i);
       // printf("\n TLB ACCESS STATS\n Number of L1 TLB hits: %d\n", proc_access_info[i].num_l1_tlb_hits);
       // printf(" Number of L1 TLB misses:%d\n", proc_access_info[i].num_l1_tlb_misses);
       // printf(" Total number of L1 TLB accesses: %d\n", proc_access_info[i].num_l1_tlb_accesses);
       
       // printf("\n L1 (Way-Halting) CACHE ACCESS STATS\n Number of L1 Cache hits: %d\n", proc_access_info[i].num_l1_cache_hits);
       // printf(" Number of L1 Cache misses: %d\n", proc_access_info[i].num_l1_cache_misses);
       // printf(" Number of L1 Cache predetermined misses: %d\n", proc_access_info[i].num_l1_cache_predetermined_misses);
       // printf(" Total number of L1 Cache accesses: %d\n", proc_access_info[i].num_l1_cache_accesses);
              
       // printf("\n L2 CACHE ACCESS STATS\n Number of L1 Cache hits: %d\n", proc_access_info[i].num_l2_cache_hits);
       // printf(" Number of L2 Cache misses: %d\n", proc_access_info[i].num_l2_cache_misses);
       // printf(" Total number of L2 Cache accesses: %d\n", proc_access_info[i].num_l2cache_accesses);
       
       // printf("\n MAIN MEMORY ACCESS STATS\n Number of Main Memory hits: %d\n", proc_access_info[i].num_main_memory_hits);
       // printf(" Number of Main Memory misses: %d\n", proc_access_info[i].num_main_memory_misses);
       // printf(" Total number of Main Memory accesses: %d\n", proc_access_info[i].num_main_memory_accesses);
       
       // printf("\n Page Fault Frequency (on average): %lf\n", proc_access_info[i].page_fault_frequency);  
   }
   
   // Calculating the hit rates/ other access parameters at each level of memory subsystem
   for (i = 0; i < num_processes; i++) {
       tlb_L1_hit_rate = tlb_L1_hit_rate + ((double)proc_access_info[i].num_l1_tlb_hits / (double)proc_access_info[i].num_l1_tlb_accesses);
       tlb_L2_hit_rate = tlb_L2_hit_rate + ((double)proc_access_info[i].num_l2_tlb_hits / (double)proc_access_info[i].num_l2_tlb_accesses);
       L1_cache_hit_rate = L1_cache_hit_rate + ((double)proc_access_info[i].num_l1_cache_hits / (double)proc_access_info[i].num_l1_cache_accesses);
       L2_cache_hit_rate = L2_cache_hit_rate + ((double)proc_access_info[i].num_l2_cache_hits / (double)proc_access_info[i].num_l2_cache_accesses);
       // Main_Memory_hit_rate = Main_Memory_hit_rate + ((double)proc_access_info[i].num_main_memory_hits / (double)proc_access_info[i].num_main_memory_accesses);
       page_fault_frequency = page_fault_frequency + ((double)proc_access_info[i].num_main_memory_misses/(double)proc_access_info[i].num_main_memory_accesses);
   } 
   
   // printf("\n Printing System Access Parameters\n")
   // printf(" L1 TLB hit rate: %lf\n", tlb_L1_hit_rate);
   // printf(" L2 TLB hit rate: %lf\n", tlb_L2_hit_rate);
   // printf(" L1 Cache hit rate: %lf\n", L1_cache_hit_rate);
   // printf(" L2 Cache hit rate: %lf\n", L2_cache_hit_rate);
   // printf(" Average page fault frequency (rate): %lf\n", page_fault_frequency);
   
   // Free 
   main_memory_free(mm_ptr);
   free(l1_instr_cache);
   free(l1_data_cache);
   free(l1_tlb);
   free(l2_tlb);
   
   return 0;
}


