#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include "pagetable.h"

typedef struct frame_table_entry
{
    int frame_number;
    int pid;
    int page_number;
} frame_table_entry;

typedef struct frame_table
{
    frame_table_entry* entry_table[65536];
} frame_table;


typedef struct main_memory_block
{
    int entry[64];
} main_memory_block;

typedef struct main_memory
{
    main_memory_block* blocks[65536];
    frame_table f_table;
} main_memory;

#endif