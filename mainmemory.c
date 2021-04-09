#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"

main_memory* mm;
typedef struct 
{
    int data;
} l2_cache_block;

main_memory* main_memory_initialize()
{
    main_memory* mm;
    mm = (main_memory*)malloc(sizeof(main_memory));
    return mm;
}

frame_table* frame_table_initialize()
{
    frame_table* f_table;
    f_table = (frame_table*)malloc(sizeof(frame_table));
    return f_table;
}
/////////////// TO BE CHANGED
l2_cache_block get_mm_block(unsigned int block_number)
{
    l2_cache_block mm_block;
    return mm_block;
}
///////////////

main_memory_block get_disk_block(unsigned int block_number, unsigned int pid)
{
    main_memory_block mm_block = *(main_memory_block*)malloc(sizeof(main_memory_block));
    //REPLACEMENT POILCY
    return mm_block;
}

void replace_mm_block()
{
    //Check end of queue
    //Change relevant page/fram tables
    //Remove node
}

unsigned int get_address(unsigned int page_number)
{
    //Get outermost page table address from pcb
    //Keep going down the page directories/tables
    //return frame number of the block; 
}

void frame_table_free(frame_table* f_table)
{
    free(f_table);
    return;
}

void main_memory_free(main_memory* mm)
{
    free(mm);
    return;
}