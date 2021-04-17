#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"

//Temp Declaration
typedef struct
{
    unsigned int pid;
    page_table* p_table_addr;
} pcb;
pcb temp_pcb;
main_memory* mm;
//

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
    // TODO: Outermost level one page with 4 entries
    // DONE

    page_table* outer = temp_pcb.p_table_addr;
    unsigned int middle, inner;
    unsigned int outerindex = block_number >> 14;
    // 10 addresses map to 00 and 01
    if(outerindex==32) middle = outer->entry_table[0].pageframe.p_table_index;
    else if(outerindex==33) middle = outer->entry_table[1].pageframe.p_table_index;
    // 7f addresses map to 10 and 11
    else if(outerindex==254) middle = outer->entry_table[2].pageframe.p_table_index; //Never occurs
    else if(outerindex==255) middle = outer->entry_table[3].pageframe.p_table_index;

    else //INVALID entry
    {
        // printf("invalid entry\n");
        return NULL;
    }

    // Middle level
    // Each entry points to a page directory with 128 entries each
    unsigned int middleindex = (block_number >> 7)%128;
    inner = mm->p_tables[middle]->entry_table[middleindex].pageframe.p_table_index;
    // Innermost level
    // Each entry in outermost level points to a block containing 128 entries each
    unsigned int frameindex = block_number % 128;
    main_memory_block* retval;
    //Go to the table whose adress was found (inner), get the required entry (frameindex) and get frame number
    unsigned int frame_address = mm->p_tables[inner]->entry_table[frameindex].pageframe.frame_num;
    retval = mm->blocks[frame_address];
    // Each of these entries points to a block/frame, return this pointer.

    return retval;
}

void page_table_free(page_table* p_table)
{
    free(p_table);
    return;
}