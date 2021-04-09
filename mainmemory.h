#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#define VALID 1
#define INVALID 0

#include "pagetable.h"

typedef struct
{
    unsigned int frame_number:16;
    unsigned int pid:16;
    unsigned int page_number:23;
    unsigned int valid_bit:1;
} frame_table_entry;

typedef struct 
{
    frame_table_entry* entry_table[65536];
} frame_table;


typedef struct 
{
    unsigned int entry[64];
} main_memory_block;

typedef struct
{
    frame_table f_table;
    page_table* p_tables[65536]; //number of page tables in memory. Could be changed later on.//
    main_memory_block* blocks[65536];
} main_memory;

typedef struct 
{
    unsigned int block_number:16;
    main_memory_block* data;
    second_chance_node* prev;
    second_chance_node* next;
    unsigned int second_chance_bit:1;
} second_chance_node;

#endif