#ifndef L2_CACHE    
#define L2_CACHE 

#define NUM_L2_CACHE_WAYS 16
#define NUM_L2_CACHE_SETS 32
#define NUM_L2_CACHE_SET_INDEX_BITS 5  

#define VALID 1
#define INVALID 0

typedef struct l2_cache_entry
{
    unsigned int tag:5;
    unsigned int data:512;
    unsigned int valid_bit:1;
    unsigned int fifo_bits:4;
} l2_cache_entry;

typedef struct set_entry 
{
    l2_cache_entry set_entries[16];
} way_entry;

typedef struct l2_cache
{
    set_entry set_array[32];//32 sets must be there
} l2_cache;



#endif