#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"
#include "mainmemory.h"

#define PAGE_TABLE_LIMIT 1019
#define DIRECTORY 1
#define TABLE 0

//Temp Declaration
typedef struct
{
    unsigned int pid;
    page_table* p_table_addr;
} pcb;
// pcb temp_pcb;
main_memory* mm;
page_table_lru_queue page_table_lru;
int total_page_count;
int page_table_index;
int frame_table_index;
//

extern main_memory_block get_disk_block(unsigned int block_number, unsigned int pid);

page_table_lru_queue page_table_lru_init()
{
    page_table_lru.head = (page_table_lru_node*)malloc(sizeof(page_table_lru_node));
    page_table_lru.tail = (page_table_lru_node*)malloc(sizeof(page_table_lru_node));
    page_table_lru.head->next = page_table_lru.tail;
    page_table_lru.head->prev = NULL;
    page_table_lru.tail->next = NULL;
    page_table_lru.tail->prev = page_table_lru.head;
    return page_table_lru;
}

void invalidate_page(unsigned int p_table_index)
{
    page_table* p_table = mm->p_tables[p_table_index];
    for(int i=0;i<128;i++)
    {
        if(p_table->entry_table[i].valid_bit==VALID)
        {
            if(p_table->granularity==1)
            {
                invalidate_page(p_table->entry_table[i].pageframe.p_table_index);
            }
            p_table->entry_table[i].valid_bit==INVALID;
        }
    }
    return;
}

void replace_page_table(page_table_lru_node* replaced)
{
    //INVALIDATE ENTRIES
    replaced->prev->next=replaced->next;
    replaced->next->prev=replaced->prev;
    free(replaced->data);
    // free(replaced->p_table_index);
    // free(replaced->start_index);
    free(replaced);
}

page_table_lru_node* page_table_init(/*should take block number as arg*/)
{
    page_table* p_table = (page_table*)malloc(sizeof(page_table));
    page_table_lru_node* ptln = (page_table_lru_node*)malloc(sizeof(page_table_lru_node));
    ptln->data = p_table;
    ptln->prev = page_table_lru.head;
    ptln->next = page_table_lru.head->next;
    page_table_lru.head->next = ptln;
    ptln->next->prev = ptln;
    page_table_index++;
    if(page_table_index>PAGE_TABLE_LIMIT)
    {
        page_table_lru_node* replaced = page_table_lru.tail->prev;
        invalidate_page(replaced->p_table_index);
        replace_page_table(replaced);
    }
    ptln->p_table_index=page_table_index;
    
    //initialize entries//
    for(int i=0;i<64;i++)
    {
        (*p_table).entry_table[i].valid_bit = INVALID;
    }
    return ptln;
    // return p_table;
}

page_table_entry* get_page_entry(unsigned int block_number /*virtual*/, pcb* temp_pcb) //page table walk
{
    page_table* outer = temp_pcb->p_table_addr;
    unsigned int middle, inner;
    unsigned int outerindex = block_number >> 14;
    // 10 addresses map to 00 and 01
    if(outerindex==32) 
    {
        if(outer->entry_table[0].valid_bit==INVALID)
        {
            // fetch page directory
            page_table_lru_node* temp = page_table_init(); //removes lru node from within
            temp->data->granularity = DIRECTORY;

            outer->entry_table[0].pageframe.p_table_index = temp->p_table_index;
        }
        middle = outer->entry_table[0].pageframe.p_table_index;
    }
    else if(outerindex==33) 
    {
        if(outer->entry_table[1].valid_bit==INVALID)
        {
            // fetch page directory
            page_table_lru_node* temp = page_table_init(); //removes lru node from within
            temp->data->granularity = DIRECTORY;

            outer->entry_table[0].pageframe.p_table_index = temp->p_table_index;
        }
        middle = outer->entry_table[1].pageframe.p_table_index;
    }
    // 7f addresses map to 10 and 11
    else if(outerindex==254) middle = outer->entry_table[2].pageframe.p_table_index; //Never occurs
    else if(outerindex==255) 
    {
        if(outer->entry_table[3].valid_bit==INVALID)
        {
            // fetch page directory
            page_table_lru_node* temp = page_table_init(); //removes lru node from within
            temp->data->granularity = DIRECTORY;

            outer->entry_table[0].pageframe.p_table_index = temp->p_table_index;
        }
        middle = outer->entry_table[3].pageframe.p_table_index;
    }
    else //INVALID entry
    {
        // printf("invalid entry\n");
        return NULL;
    }

    // Middle level
    // Each entry in outer level points to a page directory with 128 entries each
    unsigned int middleindex = (block_number >> 7)%128;
    if(mm->p_tables[middle]->entry_table[middleindex].valid_bit==INVALID)
    {
        //fetch page table
        page_table_lru_node* temp = page_table_init(); //removes lru node from within
        temp->data->granularity = TABLE;

        mm->p_tables[middle]->entry_table[0].pageframe.p_table_index = temp->p_table_index;
    }
    inner = mm->p_tables[middle]->entry_table[middleindex].pageframe.p_table_index;
    // Innermost level
    // Each entry in middle level points to a page table containing 128 entries each
    unsigned int frameindex = block_number % 128;
    page_table_entry* retval;

    //Go to the table whose adress was found (inner), get the required entry (frameindex), return its pointer
    if(mm->p_tables[inner]->entry_table[frameindex].valid_bit==INVALID)
    {
        //fetch block
        unsigned int index = frame_table_index;
        while(mm->f_table.entry_table[index]->valid_bit==VALID)
        {
            index++;
        }
        main_memory_block temp = get_disk_block(index, temp_pcb->pid);
        mm->f_table.entry_table[index]->page_number=block_number;
    }
    retval = &(mm->p_tables[inner]->entry_table[frameindex]);

    return retval;
}

main_memory_block* get_address(unsigned int block_number, pcb* temp_pcb)
{
    //Get page table entry;
    page_table_entry* page_entry = get_page_entry(block_number, temp_pcb);
    unsigned int frame_address = page_entry->pageframe.frame_num;
    return mm->blocks[frame_address];
}

void page_table_free(page_table* p_table)
{
    free(p_table);
    return;
}