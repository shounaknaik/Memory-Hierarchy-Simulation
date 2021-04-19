
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "l2_cache.h"

int main (int argc, char *argv[]) {

    unsigned int physical_address = 0;
    unsigned int virtual_address = 0;

    data_byte* data=NULL;
    
    // To maintain TLB access stats
    int num_l2_cache_accesses = 0;
    int num_l2_cache_hits = 0;
    int num_l2_cache_misses = 0;


    data_byte write_data[64];

    
    

    srand(time(0)); 
    
    // Initialize L1 TLB
    L2_cache *l2cache_1;
    l2cache_1 = initialize_L2_cache();
    
    // Read memory requests from file
    FILE *fptr;
    fptr = fopen ("physical_addresses.txt","r");
    
    if (fptr == NULL) {
        printf(" ERROR: Could not open the input file\n");
        return -1; 
    } 
    
    for (int i = 0; i < 4; i++) {
        printf("\nITERATION %d\n\n", i);
        
        
        fscanf (fptr,"%x", &virtual_address);
        printf("Virtual address: %d\n", virtual_address);
    
        // Extract 25-bit PA from 32-bit virtual address
        physical_address = virtual_address >> 7;
        printf("Physical address: %d\n", physical_address);
    
        // Search L2 cache 
        data = search_L2_cache (l2cache_1, physical_address,write_data,READ_ACCESS);
        num_l2_cache_accesses++;

        if (data!=NULL) {
            printf("L2 Cache hit!\n");
            num_l2_cache_hits++;          
        }
        
        else { 
            printf("L2 Cache miss!\n");
            num_l2_cache_misses++;
            
        }
        print_L2_cache(l2cache_1);
    }
    return 0;
}
