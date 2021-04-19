#include <unistd.h>
#include <stdio.h>

#include "pagetable.h"
#include "mainmemory.h"
//#include "processes.h"

#define PAGE_TABLE_LIMIT 1019
#define PER_PROCESS_PAGE_LIMIT 256

///////TEMPORARY DECARATIONS TILL CODE IS INTEGRATED//
typedef struct pcb
{
    unsigned int pid:16;
    page_table* page_fir_base_addr;
    unsigned int page_count;
} pcb;
pcb* temp_pcb;

typedef struct Proc_Access_Info
{
    unsigned int pid:16;
    unsigned int num_main_memory_hits;
    unsigned int num_main_memory_misses;
} Proc_Access_Info;
Proc_Access_Info* temp_pai;
//////

main_memory* mm;
frame_table* f_table;
second_chance_fifo_queue* second_chance_fifo;
int total_page_count;
int frame_table_index;

extern page_table_entry* get_page_entry(unsigned int block_number, pcb* temp_pcb, Proc_Access_Info* temp_pai);

main_memory* main_memory_init()
{
    main_memory* mm;
    mm = (main_memory*)malloc(sizeof(main_memory));
    frame_table_index=0;
    mm->total_access_count=0;
    mm->access_hit_count=0;
    return mm;
}

second_chance_fifo_queue* second_chance_replacement_init()
{
    // second_chance_fifo_queue second_chance_fifo;
    second_chance_fifo = (second_chance_fifo_queue*)malloc(sizeof(second_chance_fifo_queue));
    second_chance_fifo->head = (second_chance_node*)malloc(sizeof(second_chance_node));
    second_chance_fifo->tail = (second_chance_node*)malloc(sizeof(second_chance_node));
    second_chance_fifo->head->next = second_chance_fifo->tail;
    second_chance_fifo->head->prev = NULL;
    second_chance_fifo->tail->next = NULL;
    second_chance_fifo->tail->prev = second_chance_fifo->head;
    return second_chance_fifo;
}

data_byte* get_l1_block(unsigned int block_number/* physical address/32 */) //called from l1 cache//
{
    unsigned int frame_number = block_number/16;
    unsigned int index = block_number%16;

    main_memory_block* temp = (mm->blocks[frame_number]);

    data_byte* data = (data_byte*)malloc(32*sizeof(data_byte));
    for(int i=0;i<32;i++)
    {
        data[i].data=temp->entry[(4*index)+i].data;
    }
    // l1_block.valid_bit = VALID;
    return data;
}

data_byte* get_l2_block(unsigned int block_number/* physical address/64 */, pcb* temp_pcb) //called from l2 cache//
{
    unsigned int frame_number = block_number/8;
    unsigned int index = block_number%8;

    main_memory_block* temp = (mm->blocks[frame_number]);

    data_byte* data = (data_byte*)malloc(64*sizeof(data_byte));
    for(int i=0;i<64;i++)
    {
        data[i].data=temp->entry[(4*index)+i].data;
    }
    // l2_block.valid_bit = VALID;
    //
    //increment hit count
    return data;
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
    // printf("replacing mm block\n");
    if(replaced->second_chance_bit==0)
    {
        //Reset valid bit in page table to zero
        //Get PID of page involved
        unsigned int temppid = f_table->entry_table[replaced->block_number]->pid;
        unsigned int page_no = f_table->entry_table[replaced->block_number]->page_number;
        // temp_pcb = &(process_table[pid]);
        //Traverse through page tables till we get to required address
        Proc_Access_Info* temp_pai= (Proc_Access_Info*)malloc(sizeof(Proc_Access_Info));
        page_table_entry* page_entry = get_page_entry(page_no, temp_pcb, temp_pai);
        free(temp_pai);
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
        second_chance_fifo->tail->prev = temp;
        //send to head of queue;
        replaced->next = second_chance_fifo->head->next;
        second_chance_fifo->head->next = replaced;
        replaced->prev = second_chance_fifo->head;
        replaced->next->prev = replaced;
        //replace_mm_block(new_replaced);
        replaced=temp;
        replace_mm_block(replaced);
    }

    mm->f_table.entry_table[replaced->block_number]->valid_bit=INVALID; //Change frame table entry//

    //temp_pcb = &(process_table[f_table->entry_table[replaced->block_number]->pid]);
    Proc_Access_Info* temp_pai = (Proc_Access_Info*)malloc(sizeof(Proc_Access_Info));
    page_table_entry* page_lookup = get_page_entry(f_table->entry_table[replaced->block_number]->page_number, temp_pcb, temp_pai);
    free(temp_pai);
    page_lookup->valid_bit=INVALID; //Change page table entry//

    return;
}

main_memory_block* get_disk_block(unsigned int block_number /*physical address/512*/, unsigned int pid)
{
    // printf("getting disk block\n");
    //increment miss count
    main_memory_block* mm_block = (main_memory_block*)malloc(sizeof(main_memory_block));
    second_chance_node* scn = (second_chance_node*)malloc(sizeof(second_chance_node));printf("malloced both\n");
    scn->data = mm_block;
    scn->block_number = block_number;
    scn->prev = second_chance_fifo->head;
    scn->next = second_chance_fifo->head->next;
    second_chance_fifo->head->next = scn;
    scn->next->prev = scn; 
    scn->second_chance_bit=1;
    mm->f_table.entry_table[block_number] = (frame_table_entry*)malloc(sizeof(frame_table_entry));
    mm->f_table.entry_table[block_number]->valid_bit=VALID;
    mm->f_table.entry_table[block_number]->frame_number=block_number;
    mm->f_table.entry_table[block_number]->pid=pid;

    total_page_count++;
    temp_pcb->page_count++;
    // printf("counts incremented\n");
    if(total_page_count>PAGE_TABLE_LIMIT)
    {
        second_chance_node* replaced = second_chance_fifo->tail->prev;
        replace_mm_block(replaced);
        total_page_count--;
    }
    else if(temp_pcb->page_count >= PER_PROCESS_PAGE_LIMIT)
    {
        second_chance_node* replaced = second_chance_fifo->tail->prev;
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
                    
                    replaced->next = second_chance_fifo->head->next;
                    second_chance_fifo->head->next = replaced;
                    replaced->prev = second_chance_fifo->head;
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
    // printf("finished\n");
    return mm_block;
}

// unsigned int get_data_address(unsigned int page_number)
// Check in pagetabe.c

void frame_table_free(frame_table* f_table)
{
    free(f_table);
    return;
}

void second_chance_free(second_chance_fifo_queue* second_chance_fifo)
{
    second_chance_node* curr = second_chance_fifo->head;
    second_chance_node* clear;
    while (curr->next != second_chance_fifo->tail)
    {
        clear = curr;
        curr = curr->next;
        free(clear);
    }
    free (second_chance_fifo);
    return;
}

void main_memory_free(main_memory* mm)
{
    frame_table_free(&(mm->f_table));
    second_chance_free(second_chance_fifo);
    for(int i=0;i<1024;i++)
    {
        if(mm->p_tables[i]!=NULL) free(mm->p_tables[i]);
    }
    for(int i=0;i<63488;i++)
    {
        if(mm->blocks[i]!=NULL) free(mm->blocks[i]);
    }
    free(mm);
    return;
}
