#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"

main_memory* mm;

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

main_memory_block get_mm_block(unsigned int block_number)
{
    main_memory_block mm_block = *(mm->blocks[block_number]);
    return mm_block;
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