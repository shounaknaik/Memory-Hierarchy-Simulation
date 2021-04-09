#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"
//#include "l2_cache.h"

main_memory* mm;
typedef struct
{
    unsigned int pid;
    unsigned int p_table_addr;
} pcb;
typedef struct 
{
    unsigned int tag:5;
    unsigned int data[16];
    unsigned int valid_bit:1;
    unsigned int fifo_bits:4;
} l2_cache_entry;

main_memory* main_memory_init()
{
    main_memory* mm;
    mm = (main_memory*)malloc(sizeof(main_memory));
    return mm;
}

second_chance_fifo_queue second_chance_replacement_init()
{
    second_chance_fifo_queue second_chance_fifo;
    second_chance_fifo.head = (second_chance_node*)malloc(sizeof(second_chance_node));
    second_chance_fifo.tail = (second_chance_node*)malloc(sizeof(second_chance_node));
    second_chance_fifo.head->next = second_chance_fifo.tail;
    second_chance_fifo.head->prev = NULL;
    second_chance_fifo.tail->next = NULL;
    second_chance_fifo.tail->prev = second_chance_fifo.head;
    return second_chance_fifo;
}

frame_table* frame_table_init()
{
    frame_table* f_table;
    f_table = (frame_table*)malloc(sizeof(frame_table));
    return f_table;
}

l2_cache_entry get_mm_block(unsigned int block_number)
{
    unsigned int frame_number = block_number/8;
    unsigned int index = block_number%8;

    unsigned int temp_data[16];

    main_memory_block* temp = (mm->blocks[frame_number]);

    l2_cache_entry l2_block;
    for(int i=0;i<16;i++)
    {
        l2_block.data[i]=temp->entry[index+i];
    }
    l2_block.valid_bit = VALID;
    return l2_block;
}

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
    pcb temppcb;
    int start_addr = temppcb.p_table_addr;
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