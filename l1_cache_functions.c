// Is there a simpler way to get WRITE-BACK address?
// Check all counter updation functions

#include <stdio.h>
#include <stdlib.h>
// #include "l1_cache.h"
#include "cache.h"

/* Creates an empty L1 cache structure with the given specifications and initializes all the entries. */

L1_cache* initialize_L1_cache (int cache_type) {

    // Create an empty L1 cache structure
    L1_cache *l1_cache;
    l1_cache = (L1_cache *) malloc (sizeof (L1_cache));

    int i = 0;
    int j = 0;
    
    for (i = 0; i < NUM_L1_CACHE_SETS; i++) {
        for (j = 0; j < NUM_L1_CACHE_WAYS; j++) {
        
            // Initialize the valid bit as INVALID and dirty bit as CLEAN 
            l1_cache->l1_cache_sets[i].l1_cache_entry[j].valid_bit = INVALID;       
            l1_cache->l1_cache_sets[i].l1_cache_entry[j].dirty_bit = CLEAN;
            
            // Depending on the cache type given, initialize write bit values
            if (cache_type == INSTRUCTION)
                l1_cache->l1_cache_sets[i].l1_cache_entry[j].write_bit = READ_ONLY;    // L1 INSTUCTION cache is READ ONLY 
            else {
                l1_cache->l1_cache_sets[i].l1_cache_entry[j].write_bit = READ_WRITE;   // L1 DATA cache can be READ as well as WRITE
                // TODO: mark a section as READ_ONLY -- for Write exception to occur   // Some part of L1 DATA cache may be READ ONLY (write protection)
                // data cache writes are about 28% P&H
            }
            
            l1_cache->l1_cache_sets[i].lru_counter[j] = 0;
        }    
    }
        
    // Initialize all entries of L1 Cache Halt Tag Array (corresponding to each way) to 0
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
        for (j = 0; j < NUM_L1_CACHE_SETS; j++)
         l1_cache->halt_tag_array[i].halt_tags_per_way[j] = 0;
    }
        
    return l1_cache;
}

/* Search the L1 cache for data entry corresponding to the given physical address. If found, perform a read or write operation depeding on the type of access. */

unsigned int search_L1_cache (L1_cache* l1_cache, unsigned int physical_address, unsigned int write_data, int access_type) {
    unsigned int tag = 0;         // L1 cache tag bits
    unsigned int set_index = 0;   // L1 cache set index bits
    unsigned int offset = 0;      // L1 cache byte offset bits
    
    // Extracting the tag, offset and index bits from the physical address
    offset = physical_address % NUM_L1_CACHE_BLOCK_SIZE;
    set_index = (physical_address >> NUM_L1_CACHE_OFFSET_BITS) % NUM_L1_CACHE_SETS;
    tag = physical_address >> (NUM_L1_CACHE_SET_INDEX_BITS + NUM_L1_CACHE_OFFSET_BITS);  
    
    // L1 cache is Way-halting - so tag is further divided into main tag and halt tag
    unsigned int halt_tag = 0;    // low-order 4 bits give the halt tag
    unsigned int main_tag = 0;    // remaining tag bits (tag bits - 4 halt tag bits) give the main tag 

    // Extracting the main tag and halt tag entries from the tag field
    main_tag = tag >> NUM_L1_CACHE_HALT_TAG_BITS;
    halt_tag = tag % calc_exp(2, NUM_L1_CACHE_HALT_TAG_BITS);
    
    // While the decoder decodes the set index bits, search the halt tag array for the halt tag entry corresponding to 
    // the given physical address (in PARALLEL) -- HALT the ways corresponding to halt tag arrays with no hits.
    L1_cache_way_halting_function (l1_cache, halt_tag);

    unsigned int data;

    int i = 0;

    // Searching the L1 cache
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {

        // Check halt tag entries corresponding to ACTIVE ways and the decoded set index - if entry matches proceed to compare main tags 
        if (l1_cache->way_status[i] == ACTIVE && l1_cache->halt_tag_array[i].halt_tags_per_way[set_index] == halt_tag) {
        
            // If the entry is VALID and the tag value matches the main tag bits -- entry found!        
            if (l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].valid_bit == VALID && l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].main_tag_bits == main_tag) {
                    
                // For READ access
                if (access_type == READ_ACCESS) {
 
                    // Read 1 byte of data from L1 cache datablock (location in datablock given by offset)
                    data = l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].data_blocks[offset].data;     
 
                    // Update LRU counter corresponding to this set index
                    // update_LRU_counter(l1_cache, set_index, i);
                    update_L1_LRU_counter(l1_cache, set_index, i);    
                    
                    return data;
                }
                                
                // For WRITE access - when WRITE permission is available
                else if (access_type == WRITE_ACCESS && l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].write_bit == READ_WRITE) {
                    
                    // Write 1 byte of data into L1 cache datablock (location in datablock given by offset)
                    l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].data_blocks[offset].data = write_data;
                    
                    // Set the dirty bit to indicate that the data is modified in L1 cache (write-back policy)
                    l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].dirty_bit = DIRTY;
                    
                    // Update LRU counter corresponding to this set index
                    update_LRU_counter(l1_cache, set_index, i);
                        
                    return L1_CACHE_WRITE_SUCCESSFUL; 
                }
                
                // For WRITE access - when WRITE permission is denied --- WRITE PROTECTION EXCEPTION
                else if (access_type == WRITE_ACCESS && l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].write_bit == READ_ONLY) {
                    return L1_CACHE_WRITE_PROTECTION_EXCEPTION; // CONTEXT SWITCH triggered TODO
                }
            }
        }
    }
    
    // For L1 cache miss
    return L1_CACHE_MISS;    // Search L2 and main memory (L1 - look-through and L2 - look-aside), update L1
}

/* Performs the way-halting function for L1 cache by searching all the halt tag arrays for the halt tag corresponding to the given physical address (in PARALLEL with set index decoding). If no match is found, the corresponding way is a predetermined MISS. So, it HALTs the corresponding way to prevent unnecessary access, thereby saving energy. */

void L1_cache_way_halting_function (L1_cache* l1_cache, unsigned int halt_tag) {
    int halt_tag_comparator[NUM_L1_CACHE_SETS];    // halt tag array comparator output bits
    int i = 0;
    int j = 0;

    // For halt tag array corresponding to each way
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
    
        // Search in PARALLEL the halt tag entries corresponding to each set 
        for (j = 0; j < NUM_L1_CACHE_SETS; j++) {
        
            // If the j-th entry matches the halt tag from physical address 
            // j-th bit of comparator output array is set to 1, else 0
            if (l1_cache->halt_tag_array[i].halt_tags_per_way[j] == halt_tag)
                halt_tag_comparator[j] = 1;
            else
                halt_tag_comparator[j] = 0;
        }
    }
   
    // Check if any of the halt tag entries in the way halt tag arrays matches -(Actually implemented by ORing all  
    // comparator outputs to determine if output corresponding to at least one of the sets are HIGH i.e set to 1)
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
    
        // Check all entries in a given halt tag array
        for (j = 0; j < NUM_L1_CACHE_SETS; j++) {
            if (halt_tag_comparator[j] == 1)
                break;
        }
        
        // If no halt tag entries match (i.e. OR output is 0) --> predetermined MISS 
        // HALT the corresponding WAY to prevent unnecessary access 
        if (j == NUM_L1_CACHE_SETS)
            l1_cache->way_status[i] = HALTED;    
        else
            l1_cache->way_status[i] = ACTIVE; 
    }
}


/* Updates the L1 cache with the datablock fetched from the next level i.e. L2 cache and main memory (look-aside). Depending on the availability of free slots an entry is PLACED/REPLACED in the L1 cache. If an INVALID entry is available in any of the ways corresponding to the required set index, PLACE the new entry there. Else, REPLACE any of the way entries with required set index using LRU REPLACEMENT. */

void update_L1_cache (L1_cache* l1_cache, L2_cache* l2_cache, unsigned int *data, unsigned int physical_address) {
// void update_L1_cache (L1_cache* l1_cache, L2_cache* l2_cache, data_byte *data, unsigned int physical_address) {
    unsigned int tag = 0;         // L1 cache tag bits
    unsigned int set_index = 0;   // L1 cache set index bits
    unsigned int offset = 0;      // L1 cache byte offset bits
    
    // Extracting the tag, offset and index bits from the physical address
    offset = physical_address % NUM_L1_CACHE_BLOCK_SIZE;
    set_index = (physical_address >> NUM_L1_CACHE_OFFSET_BITS) % NUM_L1_CACHE_SETS;
    tag = physical_address >> (NUM_L1_CACHE_SET_INDEX_BITS + NUM_L1_CACHE_OFFSET_BITS);  
    
    unsigned int halt_tag = 0;    // low-order 4 bits give the halt tag
    unsigned int main_tag = 0;    // remaining tag bits (tag bits - 4 halt tag bits) give the main tag 
    
    // Extracting the main tag and halt tag entries from the tag field
    main_tag = tag >> NUM_L1_CACHE_HALT_TAG_BITS;
    halt_tag = tag % calc_exp(2, NUM_L1_CACHE_HALT_TAG_BITS); // to extract the low-order halt tag bits from tag 

    int LRU_way = 0;              // way index corresponding to least recently used entry in the set

    int i = 0;
    int j = 0;

    // PLACEMENT: If there are INVALID entries in L1 cache corresponding to the given set index, PLACE this entry in the first INVALID entry's slot    
    
    // Looking for the first INVALID entry for given set index in L1 cache
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
       
        if (l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].valid_bit == INVALID) {
            
            // L1 cache data block updation
            for (j = 0; j < NUM_L1_CACHE_BLOCK_SIZE; j++)
                l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].data_blocks[j] = data[j];
            
            // L1 cache entry updation
            l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].main_tag_bits = main_tag;  
            l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].valid_bit = VALID;
            l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].dirty_bit = CLEAN;
            
            // L1 tag halt tag array entry updation
            l1_cache->halt_tag_array[i].halt_tags_per_way[set_index] = halt_tag;
            
            break;
        }        
    }
    
    // REPLACEMENT: If all entries corresponding to the given set index are VALID, check the LRU counter and replace entry corresponding to LRU way
    
    if (i == NUM_L1_CACHE_WAYS) {
    
        // Check LRU counter to get the LRU way entry index
        LRU_way = get_LRU_entry_index (l1_cache, set_index);
        
        // Check the dirty bit of selected block - if it is set, initiate write back to L2
        if (l1_cache->l1_cache_sets[set_index].l1_cache_entry[LRU_way].dirty_bit == DIRTY) {
            // TODO: WRITE-BACK TO L2 -- include L2 cache functions in the same file
            
            // Get the physical address of the dirty block to be written back to L2  
            write_back_address = (l1_cache->l1_cache_sets[set_index].l1_cache_entry[LRU_way].main_tag << (NUM_L1_CACHE_HALT_TAG_BITS + NUM_L1_CACHE_SET_INDEX_BITS + NUM_L1_CACHE_OFFSET_BITS)) |
                                 (l1_cache->halt_tag_array[LRU_way].halt_tags_per_way[set_index] << (NUM_L1_CACHE_SET_INDEX_BITS + NUM_L1_CACHE_OFFSET_BITS)) | 
                                 (set_index << NUM_L1_CACHE_OFFSET_BITS); 
            
            // Write the dirty block in L1 back to L2 cache
            search_L2_cache (l2_cache, write_back_address, data, WRITE_ACCESS);
        }
        
        // L1 cache data block updation
        for (j = 0; j < NUM_L1_CACHE_BLOCK_SIZE; j++)
            l1_cache->l1_cache_sets[set_index].l1_cache_entry[LRU_way].data_blocks[j] = data[j];
        
        // L1 cache entry updation
        l1_cache->l1_cache_sets[set_index].l1_cache_entry[LRU_way].main_tag_bits = main_tag; 
        l1_cache->l1_cache_sets[set_index].l1_cache_entry[LRU_way].valid_bit = VALID;
        l1_cache->l1_cache_sets[set_index].l1_cache_entry[LRU_way].dirty_bit = CLEAN;
        
        // L1 tag halt tag array entry updation        
        l1_cache->halt_tag_array[LRU_way].halt_tags_per_way[set_index] = halt_tag;
    }
}

/* Updates the LRU counter corresponding to given set index at every access that results in L1 cache hit. */

void update_LRU_counter (L1_cache *l1_cache, int set_index, int way_index) {
    int i = 0;
    
    // temp holds the lru count of accessed way index before updation
    int temp = l1_cache->l1_cache_sets[set_index].lru_counter[way_index]; 
    
    // Update LRU counter values for all ways
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
    
        // For the way index accessed, set the counter to maximum value i.e. NUM_L1_CACHE_WAYS - 1 
        if (i == way_index)
            l1_cache->l1_cache_sets[set_index].lru_counter[way_index] = (NUM_L1_CACHE_WAYS - 1);
            
        // For all the other ways, decrement LRU counter by 1 if it is greater than temp 
        else {
            if (l1_cache->l1_cache_sets[set_index].lru_counter[i] > temp) 
                l1_cache->l1_cache_sets[set_index].lru_counter[i]--;   
        }
    }         
}

/* Get the LRU way entry index for REPLACEMENT */

int get_LRU_entry_index (L1_cache* l1_cache, int set_index) {
    int i = 0;
    
    // Check LRU counter values for all ways to find the first way entry having counter value as 0
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
        if (l1_cache->l1_cache_sets[set_index].lru_counter[i] == 0)
            break;        
    }
    return i;
}

/* Prints L1 instruction/data cache entries, halt tag arrays and data blocks */

void print_L1_cache (L1_cache *l1_cache) {

    int i = 0;
    int j = 0;
    int k = 0;

    for (i = 0; i < NUM_L1_CACHE_SETS; i++) {
        printf ("SET %d\n", i + 1);
        for (j = 0; j < NUM_L1_CACHE_WAYS; j++) {
            printf(" Main Tag: %d Valid Bit: %d Dirty bit: %d\n Write bit: %d\n", l1_cache->l1_cache_sets[i].l1_cache_entry[j].main_tag_bits, l1_cache->l1_cache_sets[i].l1_cache_entry[j].valid_bit, l1_cache->l1_cache_sets[i].l1_cache_entry[j].dirty_bit, l1_cache->l1_cache_sets[i].l1_cache_entry[j].write_bit);
            printf(" LRU counter for set %d: %d\n", i + 1, l1_cache->l1_cache_sets[i].lru_counter[j]); 
            for (k = 0;  k < NUM_L1_CACHE_BLOCK_SIZE; k++)
                printf(" Data block[%d]: %d ", k, l1_cache->l1_cache_sets[i].l1_cache_entry[j].data_blocks[k]);
            printf("\n\n");
        }
  
    }
        
    // Initialize all entries of L1 Cache Halt Tag Array (corresponding to each way) to 0
    for (i = 0; i < NUM_L1_CACHE_WAYS; i++) {
        printf (" Halt tag array for way %d\n", i + 1);
        for (j = 0; j < NUM_L1_CACHE_SETS; j++)
            printf (" Halt tag[%d]: %d ", j, l1_cache->halt_tag_array[i].halt_tags_per_way[j]);
    }
}


/* Calculates a^b */

int calc_exp (int a, int b) {
    int i = 0;
    for (i = 0; i < b - 1; i++)
        a = a * a;
    return a;
}


