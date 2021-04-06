#ifndef PAGE_TABLE_H    
#define PAGE_TABLE_H

typedef struct page_table_entry
{
    int virtual_address;
    int physical_address;
    int valid_bit;
    int second_cahnce_bit;
} page_table_entry;

typedef struct page_table
{
    page_table_entry entry_table[64];
} page_table;

#endif