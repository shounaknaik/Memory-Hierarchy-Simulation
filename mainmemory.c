#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "pagetable.h"
#include "mainmemory.h"
//#include "l2_cache.h"

#define PAGE_TABLE_LIMIT 1024
#define PER_PROCESS_PAGE_LIMIT 256

///////TEMPORARY DECARATIONS TILL CODE IS INTEGRATED//
typedef struct pcb
{
    unsigned int pid:16;
    page_table* p_table_addr;
    unsigned int page_count;
} pcb;
pcb* temp_pcb;

typedef struct l2_cache_block
{
    unsigned int tag:5;
    data_byte data[64];
    unsigned int valid_bit:1;
    unsigned int fifo_bits:4;
} l2_cache_block;

typedef struct l1_cache_block
{
    unsigned int tag:5;
    data_byte data[32];
    unsigned int valid_bit:1;
    unsigned int fifo_bits:4;
} l1_cache_block;

main_memory* mm;
frame_table* f_table;
second_chance_fifo_queue second_chance_fifo;
int total_page_count;
int frame_table_index;
//////

extern page_table_entry* get_page_entry(unsigned int block_number, pcb* temp_pcb);

void frame_table_init(frame_table* f_table)
{
    // frame_table* f_table;
    // f_table = (frame_table*)malloc(sizeof(frame_table));
    for(int i=0;i<63488;i++)
    {
        f_table->entry_table[i]->frame_number=i;
        f_table->entry_table[i]->valid_bit=INVALID;
    }
    return f_table;
}

main_memory* main_memory_init()
{
    main_memory* mm;
    mm = (main_memory*)malloc(sizeof(main_memory));
    frame_table_init(&(mm->f_table));
    frame_table_index=0;
    mm->total_access_count=0;
    mm->access_hit_count=0;
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

l1_cache_block get_l1_block(unsigned int block_number/* physical address/32 */) //called from l1 cache//
{
    unsigned int frame_number = block_number/16;
    unsigned int index = block_number%16;

    main_memory_block* temp = (mm->blocks[frame_number]);

    l1_cache_block l1_block;
    for(int i=0;i<32;i++)
    {
        l1_block.data[i].data=temp->entry[(4*index)+i].data;
    }
    l1_block.valid_bit = VALID;
    return l1_block;
}

l2_cache_block get_l2_block(unsigned int block_number/* physical address/64 */) //called from l2 cache//
{
    unsigned int frame_number = block_number/8;
    unsigned int index = block_number%8;

    main_memory_block* temp = (mm->blocks[frame_number]);

    l2_cache_block l2_block;
    for(int i=0;i<64;i++)
    {
        l2_block.data[i].data=temp->entry[(4*index)+i].data;
    }
    l2_block.valid_bit = VALID;
    //increment access count
    //increment hit count
    return l2_block;
}

void write_to_main_memory(unsigned int physical_address/*actual physcal address*/, data_byte* write_data) //called from l2 cache
{
    unsigned int frame_number=physical_address/512;
    unsigned int byte_offset=physical_address%512;
    
    main_memory_block* temp = (mm->blocks[frame_number]);

    for(int i=0;i<64;i++)
    {
        temp->entry[byte_offset/8+i]=write_data[i];
    }
    return;
}

void replace_mm_block(second_chance_node* replaced) //functionality of second_chance_fifo//
{
    //Check end of queue
    printf("replacing mm block\n");
    if(replaced->second_chance_bit==0)
    {
        //Reset valid bit in page table to zero
        //Get PID of page involved
        unsigned int temppid = f_table->entry_table[replaced->block_number]->pid;
        unsigned int page_no = f_table->entry_table[replaced->block_number]->page_number;
        // temp_pcb = &(process_table[pid]);
        //Traverse through page tables till we get to required address
        page_table_entry* page_entry = get_page_entry(page_no, temp_pcb);
        //Set valid bit to 0
        page_entry->valid_bit=INVALID;
        //Remove node from fifo structure
        replaced->prev->next=replaced->next;
        replaced->next->prev=replaced->prev;

        //Remove node
        free(replaced->data);
        // free(replaced->second_chance_bit);
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

    mm->f_table.entry_table[replaced->block_number]->valid_bit=INVALID; //Change frame table entry//

    //temp_pcb = &(process_table[f_table->entry_table[replaced->block_number]->pid]);
    page_table_entry* page_lookup = get_page_entry(f_table->entry_table[replaced->block_number]->page_number, temp_pcb);
    page_lookup->valid_bit=INVALID; //Change page table entry//

    return;
}

main_memory_block get_disk_block(unsigned int block_number, unsigned int pid)
{
    printf("getting disk block\n");
    //increment access count
    //increment miss count
    main_memory_block mm_block = *(main_memory_block*)malloc(sizeof(main_memory_block));
    second_chance_node* scn = (second_chance_node*)malloc(sizeof(second_chance_node));
    scn->data = &mm_block;
    scn->block_number = block_number;
    scn->prev = second_chance_fifo.head;
    scn->next = second_chance_fifo.head->next;
    second_chance_fifo.head->next = scn;
    scn->next->prev = scn;
    scn->second_chance_bit=1;
    mm->f_table.entry_table[block_number]->valid_bit=VALID;
    mm->f_table.entry_table[block_number]->frame_number=block_number;
    mm->f_table.entry_table[block_number]->pid=pid;

    total_page_count++;
    temp_pcb->page_count++;
    
    if(total_page_count>PAGE_TABLE_LIMIT)
    {
        second_chance_node* replaced = second_chance_fifo.tail->prev;
        replace_mm_block(replaced);
        total_page_count--;
    }
    else if(temp_pcb->page_count >= PER_PROCESS_PAGE_LIMIT)
    {
        second_chance_node* replaced = second_chance_fifo.tail->prev;
        while(1)
        {
            if(mm->f_table.entry_table[replaced->block_number]->pid == pid)
            {
                if(replaced->second_chance_bit)
                {
                    replaced->second_chance_bit=0;
                    
                    second_chance_node* temp = replaced->prev;
                    temp->next = replaced->next;
                    replaced->next->prev = temp;
                    
                    replaced->next = second_chance_fifo.head->next;
                    second_chance_fifo.head->next = replaced;
                    replaced->prev = second_chance_fifo.head;
                    replaced->next->prev = replaced;
                    
                    replaced = temp;
                    continue;
                }
                else break;
            }
            replaced = replaced->prev;
        }
        replace_mm_block(replaced);
        total_page_count--;
        temp_pcb->page_count--;
    }

    return mm_block;
}

// unsigned int get_data_address(unsigned int page_number)
// Check in pagetabe.c

void frame_table_free(frame_table* f_table)
{
    free(f_table);
    return;
}

void main_memory_free(main_memory* mm)
{
    frame_table_free(&(mm->f_table));
    free(mm);
    return;
}
