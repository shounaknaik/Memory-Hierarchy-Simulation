#ifndef L2_CACHE    
#define L2_CACHE 

#define NUM_L2_CACHE_WAYS 16
#define NUM_L2_CACHE_SETS 32
#define NUM_L2_CACHE_SET_INDEX_BITS 5  

#define VALID 1
#define INVALID 0

#define READ_ACCESS 0
#define WRITE_ACCESS 1

typedef struct l2_cache_entry
{
    unsigned int tag:14;
    data_byte data[64];
    unsigned int valid_bit:1;
    unsigned int fifo_bits:4;
} l2_cache_entry;

typedef struct {
    unsigned int data:8;                                 // Each data entry in cache block is 1B = 8 bits
} data_byte;

typedef struct set_entry 
{
    l2_cache_entry set_entries[16];
} set_entry;

typedef struct l2_cache
{
    set_entry set_array[32];//32 sets must be there
} l2_cache;



#endif