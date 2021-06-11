#ifndef CACHE_H   
#define CACHE_H


/////////////////////// L1 CACHE macros //////////////////////////

// 2KB 4-way set-associative L1 cache
#define NUM_L1_CACHE_WAYS 4
#define NUM_L1_CACHE_SETS 16
#define NUM_L1_CACHE_BLOCK_SIZE 32           // in bytes
  
#define NUM_L1_CACHE_SET_INDEX_BITS 4        // L1 cache index bits -- No. of bits required to address a particular set/ total 16 sets -- log_2(16)
#define NUM_L1_CACHE_OFFSET_BITS 5           // L1 cache offset bits -- No. of bits required to address a byte in 32 byte block -- log_2(32)
#define NUM_L1_CACHE_HALT_TAG_BITS 4         // L1 cache halt tag bits -- Way-halting cache maintains low-order tag bits in a separate structure to predetermine misses 
#define NUM_L1_CACHE_MAIN_TAG_BITS 12        // L1 cache tag bits -- (No. of bits in frame address - L1 cache index bits - L1 cache offset bits - L1 cache halt tag bits)       

#define NUM_L1_LRU_COUNTER_BITS 8            // No. of bits required to maintain an LRU counter for 4 ways -- 4.log_2(4)

// Valid bit values
#define INVALID 0
#define VALID 1

// Dirty bit values
#define CLEAN 0
#define DIRTY 1

// Read write bit values
#define READ_ONLY 0
#define READ_WRITE 1

// To specify L1 cache type
#define INSTRUCTION 0
#define DATA 1

// To specify L1 way status
#define ACTIVE 0
#define HALTED 1

// To specify the operation to be performed
#define READ_ACCESS 0
#define WRITE_ACCESS 1

// To specify access status
#define L1_CACHE_WRITE_SUCCESSFUL 256                    // using numbers greater than 255 (max valid value of data that can be returned) to specify L1 cache access status 
#define L1_CACHE_WRITE_PROTECTION_EXCEPTION 257
#define L1_CACHE_MISS 258
#define L1_CACHE_MISS_PREDETERMINED 259

/////////////////////// L2 CACHE macros //////////////////////////

#define NUM_L2_CACHE_WAYS 16
#define NUM_L2_CACHE_SETS 32
#define NUM_L2_CACHE_SET_INDEX_BITS 5  

#define NUM_L2_CACHE_BLOCK_SIZE 64  
#define NUM_L2_CACHE_OFFSET_BITS 6    


/////////////////////// L1 CACHE structures //////////////////////////

// L1 cache entry structure 

typedef struct {
    unsigned int data:8;                                 // Each data entry in cache block is 1B = 8 bits
} data_byte;

typedef struct {
    unsigned int main_tag_bits:11;                       // First 11 bits of 15-bit cache tag bits - stored as main tag bits with each entry 
    unsigned int valid_bit:1;                            // Valid bit - set if the corresponding entry is valid
    unsigned int dirty_bit:1;                            // Dirty bit - set if the corresponding entry has been modified in this level but not updated in the next level
    unsigned int write_bit:1;                            // Read-Write permissions for the data block
    // unsigned int data[NUM_L1_CACHE_BLOCK_SIZE];       // 32B data block - assume 1B data stored in each array element /////////// data <-- void *? any way to specify an array of bit-fields?
    data_byte data_blocks [NUM_L1_CACHE_BLOCK_SIZE];
} L1_cache_entry;

// L1 cache structures

typedef struct {
    L1_cache_entry l1_cache_entry[NUM_L1_CACHE_WAYS];     // Array of NUM_L1_CACHE_WAYS L1 cache entries per set
    unsigned int lru_counter[NUM_L1_CACHE_WAYS];          // N.log(N) bit LRU counter is maintained per set - assume log(N) bits stored in each array element N - No. of ways 
} L1_cache_sets;                                          // log -- (base 2)

typedef struct {
    unsigned int halt_tags_per_way[NUM_L1_CACHE_SETS];    // Halt tag array to be maintained per way  
} Halt_tag_array;

typedef struct {
    L1_cache_sets l1_cache_sets[NUM_L1_CACHE_SETS];       // And array of NUM_L1_CACHE_SETS such sets make L1 cache
    Halt_tag_array halt_tag_array[NUM_L1_CACHE_WAYS];     // Array of NUM_L1_CACHE_WAYS halt tags -- cuz Way-Halting cache
    int way_status[NUM_L1_CACHE_WAYS];                    // Way-halting cache shuts down ways in which misses are pre-determined. Way status indicates if a particular way is HALTED or ACTIVE.
} L1_cache;

/////////////////////// L2 CACHE structures //////////////////////////

typedef struct l2_cache_entry
{
    unsigned int tag:14;
    data_byte data[64];
    unsigned int valid_bit:1;
    unsigned int fifo_bits:5;
} l2_cache_entry;

typedef struct set_entry 
{
    l2_cache_entry set_entries[16];
} set_entry;

typedef struct L2_cache
{
    set_entry set_array[32];//32 sets must be there
} L2_cache;

/////////////////////// L1 CACHE functions //////////////////////////

// Function declarations

L1_cache* initialize_L1_cache (int cache_type);
unsigned int search_L1_cache (L1_cache* l1_cache, unsigned int physical_address, unsigned int write_data, int access_type);
void L1_cache_way_halting_function (L1_cache* l1_cache, unsigned int halt_tag);
void update_L1_cache (L1_cache* l1_cache, L2_cache* l2_cache, data_byte *data, unsigned int physical_address);
void update_L1_LRU_counter (L1_cache *l1_cache, int set_index, int way_index);
int get_L1_LRU_entry_index (L1_cache* l1_cache, int set_index);
void print_L1_cache (L1_cache *l1_cache);
int calc_exp (int a, int b);

/////////////////////// L2 CACHE functions //////////////////////////

L2_cache* initialize_L2_cache ();
data_byte* search_L2_cache (L2_cache* l2_cache_1, unsigned int physical_address,data_byte* write_data, int access_type);
void update_l2_cache (L2_cache* l2_cache_1, data_byte* data,unsigned int physical_address);
void update_fifo_l2_cache(L2_cache* l2_cache_1, int set_index);
int get_FIFO_replacement(L2_cache* l2_cache_1,int set_index);
void print_L2_cache (L2_cache *l2_cache_1);

#endif
