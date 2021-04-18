#ifndef PAGE_TABLE_H    
#define PAGE_TABLE_H

#define VALID 1
#define INVALID 0

typedef union pageframe
{
    unsigned int frame_num:16;
    unsigned int p_table_index:11;
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
    unsigned int granularity:1;
} page_table;

typedef struct
{
    unsigned int start_index;
    unsigned int p_table_index:11;
    page_table* data;
    page_table_lru_node* prev;
    page_table_lru_node* next;
} page_table_lru_node;

typedef struct 
{
    page_table_lru_node* head;
    page_table_lru_node* tail;
} page_table_lru_queue;

#endif