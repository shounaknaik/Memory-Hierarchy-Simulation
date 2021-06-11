#include <stdio.h>
#include <stdlib.h>
#include "tlb.h"

/* Creates an empty L1 TLB structure with the given specifications and initializes the structure by marking all the TLB entries as INVALID. */

L1_TLB* initialize_L1_TLB () {

    // Create an empty L1 TLB structure
    L1_TLB *l1_tlb;
    l1_tlb = (L1_TLB *) malloc (sizeof (L1_TLB));

    int i = 0;
    int j = 0;
    
    // Initialize the L1 TLB with all entries marked INVALID
    for (i = 0; i < NUM_L1_TLB_SETS; i++) {
        for (j = 0; j < NUM_L1_TLB_WAYS; j++)
            l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].valid_bit = INVALID;
    }
    
    return l1_tlb;
}

/* Creates an empty L2 TLB structure with the given specifications and initializes the structure by marking all the TLB entries as INVALID, and initializing all entries of LRU square matrix as 0. */

L2_TLB* initialize_L2_TLB () {

    // Create an empty L2 TLB structure
    L2_TLB *l2_tlb;
    l2_tlb = (L2_TLB *) malloc (sizeof (L2_TLB));

    int i = 0;
    int j = 0;
    int k = 0;
    
    // Initialize the L2 TLB with all entries marked INVALID
    for (i = 0; i < NUM_L2_TLB_SETS; i++) {
        for (j = 0; j < NUM_L2_TLB_WAYS; j++)
            l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].valid_bit = INVALID;
    }
        
    // Initialize the L2 TLB LRU counters to 0
    for (i = 0; i < NUM_L2_TLB_SETS; i++) {
        for (j = 0; j < NUM_L2_TLB_WAYS; j++) {
            for (k = 0; k < NUM_L2_TLB_WAYS; k++)
                l2_tlb->l2_tlb_sets[i].lru_square_matrix[j][k] = 0;
        }
    }
    
    return l2_tlb;
}

/* Searches the L1 TLB for an entry corresponding to the given page number. If HIT returns corresponding frame number, else returns an invalid (out of range) frame number. */

unsigned int search_L1_TLB (L1_TLB* l1_tlb, unsigned int page_number) {
    int frame_number = 0;
    int i = 0;
    int set_index = page_number % NUM_L1_TLB_SETS;
    int tag = page_number >> NUM_L1_TLB_SET_INDEX_BITS;    
    
    for (i = 0; i < NUM_L1_TLB_WAYS; i++) {
    
        // If the entry is VALID and the tag value matches the page entry tag bits -- Found!
        if (l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].valid_bit == VALID && l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].page_tag_entry == tag) {
        
            // Get corresponding frame number
            frame_number = l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].frame_number_entry;
            printf ("Entry found in L1 TLB set %d, way %d Page entry (tag): %d, Frame entry (tag): %d", set_index, i+1, tag, l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].frame_number_entry);
            break;
        }        
    }
    
    // Entry found in L1 TLB -- return frame number
    if (i < NUM_L1_TLB_WAYS) 
        return frame_number;
        
    // Entry NOT found in L1 TLB
    else 
        return 6553699; // Returning an invalid frame number to indicate a miss -- anything outside 0 <= frame_number < 65536( = 2^(25-9)) but within unsigned int range works // return -73;
}

/* Searches the L2 TLB for an entry corresponding to the given page number, if HIT updates LRU square matrix and returns corresponding frame number, else returns an invalid (out of range) frame number. */
 
unsigned int search_L2_TLB (L2_TLB* l2_tlb, unsigned int page_number) {
    int frame_number = 0;
    int i = 0;
    int set_index = page_number % NUM_L2_TLB_SETS;
    int tag = page_number >> NUM_L2_TLB_SET_INDEX_BITS;
    
    for (i = 0; i < NUM_L2_TLB_WAYS; i++) {
    
        // If the entry is VALID and the tag value matches the page entry tag bits -- Found!
        if (l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].valid_bit == VALID && l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].page_tag_entry == tag) {
        
            // Get corresponding frame number
            frame_number = l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].frame_number_entry;
            printf ("Entry found in L2 TLB set %d, way %d Page entry (tag): %d, Frame entry (tag): %d", set_index, i+1, tag, l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].frame_number_entry);
            
            // Update the LRU square matrix
            for (int j = 0; j < NUM_L2_TLB_WAYS; j++)
                l2_tlb->l2_tlb_sets[set_index].lru_square_matrix[i][j] = 1;   // Row bits corresponding to accessed way made 1
            for (int j = 0; j < NUM_L2_TLB_WAYS; j++)
                l2_tlb->l2_tlb_sets[set_index].lru_square_matrix[j][i] = 0;   // Column bits corresponding to accessed way made 0  
                                
            break;
        }        
    }
    
    // Entry found in L1 TLB -- return frame number
    if (i < NUM_L2_TLB_WAYS) 
        return frame_number;
        
    // Entry NOT found in L1 TLB -- set search flag to L1_TLB_MISS
    else 
        return 6553699; // Returning an invalid frame number to indicate a miss -- anything outside 0 <= frame_number < 65536( = 2^(25-9)) but within unsigned int range works // return -73;
}

/* Updates the L1 TLB with the entry (page number, corresponding frame number) fetched from the main memory. Depending on the availability of free slots an entry is PLACED/REPLACED in the TLB. If an INVALID entry is available in any of the ways corresponding to the required set index, PLACE the new entry there. Else, REPLACE any of the way entries with required set index using RANDOM REPLACEMENT. */

void update_L1_TLB (L1_TLB* l1_tlb, unsigned int page_number, unsigned int frame_number) {
   
    int i = 0;
    int set_index = page_number % NUM_L1_TLB_SETS;
    int tag = page_number >> NUM_L1_TLB_SET_INDEX_BITS;
    int random_replacement = 0;

    // PLACEMENT: If there are INVALID entries in L1 TLB, PLACE this entry in the first INVALID entry's slot    
    // Look for INVALID entry corresponding to the given set index
    for (i = 0; i < NUM_L1_TLB_WAYS; i++) {
        if (l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].valid_bit == INVALID) {
            l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].page_tag_entry = tag; 
            l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].frame_number_entry = frame_number; 
            l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].valid_bit = VALID;
            l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[i].shared_bit = NOT_SHARED;
            break;
        }        
    }
    
    // REPLACEMENT: if all entries of the given set index are VALID, randomly REPLACE any entry
    if (i == NUM_L1_TLB_WAYS) {
        random_replacement = rand() % NUM_L1_TLB_WAYS;
        l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[random_replacement].page_tag_entry = tag; 
        l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[random_replacement].frame_number_entry = frame_number; 
        l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[random_replacement].valid_bit = VALID;
        l1_tlb->l1_tlb_sets[set_index].l1_tlb_entry[random_replacement].shared_bit = NOT_SHARED;
    }
}

/* Updates the L2 TLB with the entry (page number, corresponding frame number) fetched from the main memory. Depending on the availability of free slots an entry is PLACED/REPLACED in the TLB. If an INVALID entry is available in any of the ways corresponding to the required set index, PLACE the new entry there. Else, REPLACE the LRU way entry with required set index (way index obtained by checking the LRU square matrix corresponding to the required set index). */

void update_L2_TLB (L2_TLB* l2_tlb, unsigned int page_number, unsigned int frame_number) {
   
    int i = 0;
    int set_index = page_number % NUM_L2_TLB_SETS;
    int tag = page_number >> NUM_L2_TLB_SET_INDEX_BITS;
    int LRU_replacement = 0;

    // PLACEMENT: If there are INVALID entries in L1 TLB, PLACE this entry in the first INVALID entry's slot    
    // Look for INVALID entry corresponding to the given set index
    for (i = 0; i < NUM_L2_TLB_WAYS; i++) {
        if (l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].valid_bit == INVALID) {
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].page_tag_entry = tag; 
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].frame_number_entry = frame_number; 
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].valid_bit = VALID;
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].shared_bit = NOT_SHARED;
            break;
        }        
    }
    
    // REPLACEMENT: if all entries of the given set index are VALID, REPLACE Least Recently Used (LRU) entry
    if (i == NUM_L2_TLB_WAYS) {
        LRU_replacement = get_LRU_entry_index (l2_tlb, set_index);
        l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[LRU_replacement].page_tag_entry = tag; 
        l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[LRU_replacement].frame_number_entry = frame_number; 
        l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[LRU_replacement].valid_bit = VALID;
        l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[LRU_replacement].shared_bit = NOT_SHARED;
    }
}

/* Get the LRU way index for given set. (The LRU way index is given by the row number with all bits set to 0). */

int get_LRU_entry_index (L2_TLB* l2_tlb, int set_index) {
    int LRU_entry_index = 0;
    int zero_count = 0;
    int i = 0;
    int j = 0;
    
    // Find the first row with all 0s
    for (i = 0; i < NUM_L2_TLB_WAYS; i++) {
        zero_count = 0;
        for (j = 0; j < NUM_L2_TLB_WAYS; j++) {
            if (l2_tlb->l2_tlb_sets[set_index].lru_square_matrix[i][j] == 0)
                zero_count++;          
        }
        if (zero_count == NUM_L2_TLB_WAYS)
            break;
    }
    return i;
}

/* Flushes all L1 TLB entries (except for the ones corresponding to shared pages), by marking them as INVALID. */

void flush_L1_TLB (L1_TLB* l1_tlb) {
    int i = 0;
    int j = 0;
    
    // Flush L1 TLB with all entries (except for SHARED) -- mark INVALID
    for (i = 0; i < NUM_L1_TLB_SETS; i++) {
        for (j = 0; j < NUM_L1_TLB_WAYS; j++) {
            if (l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].shared_bit == NOT_SHARED)
                l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].valid_bit = INVALID;
        }
    }
}

/* Flushes all L2 TLB entries (except for the ones corresponding to shared pages), by marking them as INVALID and resets the LRU square matrices corresponding to all sets to 0. */
void flush_L2_TLB (L2_TLB* l2_tlb) {
    int i = 0;
    int j = 0;
    int k = 0;
    
    // Flush L2 TLB with all entries (except for SHARED) -- mark INVALID
    for (i = 0; i < NUM_L2_TLB_SETS; i++) {
        for (j = 0; j < NUM_L2_TLB_WAYS; j++) {
            if (l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].shared_bit == NOT_SHARED)
                l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].valid_bit = INVALID;
        }
    }
        
    // Reset all the L2 TLB LRU square matrices to 0
    for (i = 0; i < NUM_L2_TLB_SETS; i++) {
        for (j = 0; j < NUM_L2_TLB_WAYS; j++) {
            for (k = 0; k < NUM_L2_TLB_WAYS; k++) {
                if (l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].shared_bit == NOT_SHARED)
                    l2_tlb->l2_tlb_sets[i].lru_square_matrix[j][k] = 0;
            }
        }
    }
}

/* Prints all L1 TLB entries set-wise */

void print_L1_tlb (L1_TLB* l1_tlb) {
    int i = 0;
    int j = 0;
    
    for (i = 0; i < NUM_L1_TLB_SETS; i++) {
        printf ("SET %d\n", i + 1);
        for (j = 0; j < NUM_L1_TLB_WAYS; j++)
            printf(" Page Tag Number: %d Frame Number: %d Valid Bit: %d Shared bit: %d\n", l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].page_tag_entry, l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].frame_number_entry, l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].valid_bit, l1_tlb->l1_tlb_sets[i].l1_tlb_entry[j].shared_bit);
    }
} 

/* Prints all L2 TLB entries set-wise */

void print_L2_tlb (L2_TLB* l2_tlb) {
    int i = 0;
    int j = 0;
    
    for (i = 0; i < NUM_L2_TLB_SETS; i++) {
        printf ("SET %d\n", i + 1);
        for (j = 0; j < NUM_L2_TLB_WAYS; j++)
            printf(" Page Tag Number: %d Frame Number: %d Valid Bit: %d Shared bit: %d\n", l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].page_tag_entry, l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].frame_number_entry, l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].valid_bit, l2_tlb->l2_tlb_sets[i].l2_tlb_entry[j].shared_bit);
    }
} 

