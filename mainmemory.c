#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"
//#include "l2_cache.h"

#define MAX_PAGE_COUNT 1000

///////TEMPORARY DECARATIONS TILL CODE IS INTEGRATED//
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

typedef struct 
{
    unsigned int tag:5;
    unsigned int data[8];
    unsigned int valid_bit:1;
    unsigned int fifo_bits:4;
} l1_cache_entry;

main_memory* mm;
frame_table* f_table;
second_chance_fifo_queue second_chance_fifo;
int total_page_count;
pcb temp_pcb;
//////

extern page_table* get_address(unsigned int block_number);

main_memory* main_memory_init()
{
    main_memory* mm;
    mm = (main_memory*)malloc(sizeof(main_memory));
    return mm;
}

second_chance_fifo_queue second_chance_replacement_init()
{
    // second_chance_fifo_queue second_chance_fifo;
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

// TODO: called from l1, but return to bot l1 and l2.
// DONE.

l1_cache_entry get_l1_block(unsigned int block_number)
{
    unsigned int frame_number = block_number/16;
    unsigned int index = block_number%16;

    unsigned int temp_data[8];

    main_memory_block* temp = (mm->blocks[frame_number]);

    l1_cache_entry l1_block;
    for(int i=0;i<8;i++)
    {
        l1_block.data[i]=temp->entry[index+i];
    }
    l1_block.valid_bit = VALID;
    return l1_block;
}

l2_cache_entry get_l2_block(unsigned int block_number)
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
    second_chance_node* scn = (second_chance_node*)malloc(sizeof(second_chance_node));
    scn->data = &mm_block;
    scn->block_number = block_number;
    scn->prev = second_chance_fifo.head;
    scn->next = second_chance_fifo.head->next;
    second_chance_fifo.head->next = scn;
    scn->next->prev = scn;
    scn->second_chance_bit=1;

    total_page_count++;
    if(total_page_count>MAX_PAGE_COUNT)
    {
        second_chance_node* replaced = second_chance_fifo.tail->prev;
        replace_mm_block(replaced);
        total_page_count--;
    }
    return mm_block;
}

void replace_mm_block(second_chance_node* replaced)
{
    //Check end of queue
    if(replaced->second_chance_bit==0)
    {
        //Reset valid bit in page table to zero
        //Get PID of page involved
        unsigned int temppid = f_table->entry_table[replaced->block_number]->pid;
        //Traverse through page tables till we get to reuired address
        //Set valid bit to 0
        //Remove node from fifo structure
        replaced->prev->next=replaced->next;
        replaced->next->prev=replaced->prev;

        //Remove node
        free(replaced->data);
        free(replaced->second_chance_bit);
        free(replaced);
    }
    else
    {
        //Change bit to 0;
        replaced->second_chance_bit=0;
        //assign new_head to replaced;
        //replaced=replaced->prev;
        second_chance_node* temp = replaced->prev;
        temp->next = replaced->next;
        second_chance_fifo.tail->prev = temp;
        //send to head of queue;
        replaced->next = second_chance_fifo.head->next;
        second_chance_fifo.head->next = replaced;
        replaced->prev = second_chance_fifo.head;
        replaced->next->prev = replaced;
        //replace_mm_block(new_replaced);
        replaced=temp;
        replace_mm_block(replaced);
    }
    //TODO: Change relevant page/frame tables

    return;
}

// TODO: free frames and frame table entries.

unsigned int get_data_address(unsigned int page_number)
{
    //Get outermost page table address from pcb
    pcb temppcb;
    int start_addr = temppcb.p_table_addr;
    //Keep going down the page directories/tables
    //TODO: 
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