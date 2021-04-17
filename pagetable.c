#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"

page_table* page_table_init()
{
    page_table* p_table;
    p_table = (page_table*)malloc(sizeof(page_table));
    //initialize entries//
    for(int i=0;i<64;i++)
    {
        (*p_table).entry_table[i].valid_bit = INVALID;
    }
    return p_table;
}

main_memory_block* get_address(unsigned int block_number)
{
    // TODO:
    // Outermost level one page with 4 entries
    // 10 addresses map to 00 and 01
    // 7f addresses map to 10 and 11
    // Each entry points to a page directory with 128 entries
    // Middle level
    // Each entry points to a page table with 128 entries each
    // Innermost level
    // Each entry in outermost level points to a block containing 128 entries each
    // Each of these entries points to a block/frame, return this pointer.

    // 9 bits for frame offset
    // 7 bits page table numble
    // 7 bits page directory number
    // 9 bits (effectively 2) for outer directory number
}

void page_table_free(page_table* p_table)
{
    free(p_table);
    return;
}