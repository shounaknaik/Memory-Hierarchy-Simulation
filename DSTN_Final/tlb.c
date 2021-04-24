#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tlb.h"

int main (int argc, char *argv[]) {

    unsigned int virtual_address = 0;
    unsigned int page_number = 0;
    unsigned int frame_number = 0;
    
    // To maintain TLB access stats
    int num_l1_tlb_accesses = 0;
    int num_l1_tlb_hits = 0;
    int num_l1_tlb_misses = 0;
    
    int num_l2_tlb_accesses = 0;
    int num_l2_tlb_hits = 0;
    int num_l2_tlb_misses = 0;

    srand(time(0)); 
    
    // Initialize L1 TLB
    L1_TLB *l1_tlb;
    l1_tlb = initialize_L1_TLB ();
    
    // Initialize L2 TLB
    L2_TLB *l2_tlb;
    l2_tlb = initialize_L2_TLB ();   
    
    // Read memory requests from file
    FILE *fptr;
    fptr = fopen ("memory_requests.txt","r");
    
    if (fptr == NULL) {
        printf(" ERROR: Could not open the input file\n");
        return -1; 
    } 
    
    for (int i = 0; i < 14; i++) {
        printf("\nITERATION %d\n\n", i);
        
        // Read memory requests from input file -- determine if instr(7f)/data(10) is accessed 
        // Instruction accesses go to L1 instruction cache
        // Data accesses go to L1 data cache -- determine read/write based on rand()
        
        fscanf (fptr,"%x", &virtual_address);
        printf("Virtual address: %d\n", virtual_address);
    
        // Extract 23-bit page number from 32-bit virtual address
        page_number = virtual_address >> 9;
        printf("Page Number: %d\n", page_number);
    
        // Search L1 TLB 
        frame_number = search_L1_TLB (l1_tlb, page_number);
        num_l1_tlb_accesses++;

        // If the returned frame number is within valid range - L1 TLB HIT - frame number acquired
        if (frame_number >= 0 && frame_number <= MAX_FRAME_NUMBER) {
            printf("L1 TLB hit!\n");
            num_l1_tlb_hits++;          
        }
        
        // If the returned frame number is NOT within valid range - L1 TLB MISS
        else { 
            printf("L1 TLB miss!\n");
            num_l1_tlb_misses++;
            
            // Search L2 TLB
            frame_number = search_L2_TLB (l2_tlb, page_number);
            num_l2_tlb_accesses++;
            
            // If the returned frame number is within valid range - L2 TLB HIT - frame number acquired
            if (frame_number >= 0 && frame_number <= MAX_FRAME_NUMBER) {
                printf("L2 TLB hit!\n");
                num_l2_tlb_hits++;
            }
                
            // If the returned frame number is NOT within valid range - L2 TLB MISS
            else {
                printf("L2 TLB miss!\n");
                num_l2_tlb_misses++;
            
                // EXCEPTION: Invoke page table walk and update the TLB entry
                
                // Update L2 TLB with the acquired entry -- KERNEL
                update_L2_TLB (l2_tlb, page_number, 10*i);
                printf("L2 TLB\n");
                print_L2_tlb (l2_tlb);
                
                // Update L1 TLB with the acquired entry -- KERNEL
                update_L1_TLB (l1_tlb, page_number, 10*i);
                printf("L1 TLB\n");
                print_L1_tlb (l1_tlb);
            }
        }
    }
    return 0;
}
