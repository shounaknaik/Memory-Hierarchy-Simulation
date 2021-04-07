#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#define VALID 1
#define INVALID 0

#include "pagetable.h"

typedef struct frame_table_entry
{
    unsigned int frame_number:16;
    unsigned int pid:16;
    unsigned int page_number:23;
} frame_table_entry;

typedef struct frame_table
{
    frame_table_entry* entry_table[65536];
} frame_table;


typedef struct main_memory_block
{
    unsigned int entry[64];
} main_memory_block;

typedef struct main_memory
{
    main_memory_block* blocks[65536];
    frame_table f_table;
} main_memory;

#endif