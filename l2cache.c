#include <stdio.h>
#include <stdlib.h>
#include "l2_cache.h"


l2_cache* initialize_L2_cache () {

    // Create an empty L2 Cache structure
    l2_cache *l2_cache_1;
    l2_cache_1 = (l2_cache *) malloc (sizeof (l2_cache)); //  l1_tlb = (L1_TLB *) malloc (sizeof (L1_TLB));
    
    // Initialize the L2 cache with all entries marked INVALID
    for (int i = 0; i < NUM_L2_CACHE_SETS; i++) {
        for (int j = 0; j < NUM_L2_CACHE_WAYS ; j++)
            l2_cache_1->set_array[i].set_entries[j].valid_bit = INVALID;
    }
    
    return l2_cache_1;
}


int search_L2_cache (l2_cache* l2_cache_1, unsigned int physical_address) {

    // should search in main memory simultaneosly. As this is look aside.
    //int data=search_main_memory();
    int i=0;
    int data=0;
    int useful_data=physical_address>>6;//6 bits is byte offset
    int set_index = useful_data % NUM_L2_CACHE_SETS;
    int tag = physical_address>> NUM_L2_CACHE_SET_INDEX_BITS;    
    
    for (i = 0; i < NUM_L2_CACHE_WAYS; i++) {
    
        // If the entry is VALID and the tag value matches the page entry tag bits -- Found!
        if (l2_cache_1->set_array[set_index].set_entries[i].valid_bit == VALID && l2_cache_1->set_array[set_index].set_entries[i].tag == tag) {
        
            // Get corresponding frame number
            int data = l2_cache_1->set_array[set_index].set_entries[i].data;
            printf ("Entry found in L2 Cache");
            break;
        }        
    }
    
    // Entry found in L1 TLB -- return frame number
    if (i < NUM_L2_CACHE_WAYS) {
        // flag = L1_TLB_HIT;
        //must give signal for Main Memory search to be stopped.
        return data;
    }
    else 
    {
         //Miss . Wait for Main Memory search. 
        update_l2_cache(l2_cache_1,data,set_index);
        
         return -73;

    }
}

void update_l2_cache (l2_cache* l2_cache_1, unsigned int data,int set_index) {
   
    int i = 0; 
   

    // PLACEMENT: If there are INVALID entries in L2 Cache, PLACE this entry in the first INVALID entry's slot    
    // Look for INVALID entry corresponding to the given set index
    for (i = 0; i < NUM_L2_CACHE_WAYS; i++) {
        if (l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].valid_bit == INVALID) {
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].page_tag_entry = tag; 
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].frame_number_entry = frame_number; 
            l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[i].valid_bit = VALID;
            //must send updated data to main memory because it is write through
            break;
        }        
    }
    
    // REPLACEMENT: if all entries of the given set index are VALID, REPLACE Least Recently Used (LRU) entry
    if (i == NUM_L2_CACHE_WAYS) {
        LRU_replacement = get_LRU_entry_index (l2_tlb, set_index);
        l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[LRU_replacement].page_tag_entry = tag; 
        
        l2_tlb->l2_tlb_sets[set_index].l2_tlb_entry[LRU_replacement].valid_bit = VALID;
    }
}





