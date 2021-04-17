#ifndef PAGE_TABLE_H    
#define PAGE_TABLE_H

#define VALID 1
#define INVALID 0

typedef union pageframe
{
    unsigned int frame_num:16;
    unsigned int p_table_index;
}p_f_address;

typedef struct
{
    unsigned int page_number:23; //TODO: will be removed. page tables indexed by page number.
    p_f_address pageframe;
    unsigned int valid_bit:1;
    unsigned int shared_bit:1;
} page_table_entry;

typedef struct
{
    page_table_entry entry_table[128];
} page_table;

#endif