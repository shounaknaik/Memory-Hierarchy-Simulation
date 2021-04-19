#include <unistd.h>
#include <stdlib.h>

#include "pagetable.h"
#include "mainmemory.h"
// #include "processes.h"

#define PAGE_TABLE_LIMIT 1019
#define DIRECTORY 1
#define TABLE 0
#define SHARED 1
#define UNSHARED 0

///////TEMPORARY DECARATIONS TILL CODE IS INTEGRATED//
typedef struct pcb
{
    unsigned int pid;
    page_table* page_dir_base_addr;
    unsigned int page_count;
    unsigned int num_main_memory_hits;
    unsigned int num_main_memory_misses;
} PCB;

typedef struct Proc_Access_Info
{
    unsigned int pid:16;
    unsigned int num_main_memory_hits;
    unsigned int num_main_memory_misses;
} Proc_Access_Info;
//////

main_memory* mm;
page_table_lru_queue page_table_lru;
int total_page_count;
int page_table_index;
int frame_table_index;

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
    // printf("invalidating page");
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
    // printf("replace page table called");
    page_table_lru_node* temp = replaced->prev;
    temp->next=replaced->next;
    temp = replaced->next;
    temp->prev=replaced->prev;
    free(replaced->data);
    free(replaced);
}

page_table* page_dir_init()
{
    page_table* page_dir = (page_table*)malloc(sizeof(page_table));
    for(int i=0;i<128;i++)
    {
        page_dir->entry_table[i].valid_bit = INVALID;
    }
    page_dir->granularity = DIRECTORY;
    return page_dir;
}

page_table_lru_node* page_table_init(/*should take block number as arg*/)
{
    // printf("page table init called\n");
    // increment page access    
    // increment page miss
    page_table* p_table = (page_table*)malloc(sizeof(page_table));
    page_table_lru_node* ptln = (page_table_lru_node*)malloc(sizeof(page_table_lru_node));
    ptln->data = p_table;
    ptln->prev = page_table_lru.head;
    ptln->next = page_table_lru.head->next;
    page_table_lru.head->next = ptln;
    page_table_lru_node* temp = ptln->next;
    temp->prev = ptln;
    page_table_index++;
    total_page_count++;
    frame_table_index++;
    if(page_table_index>PAGE_TABLE_LIMIT)
    {
        page_table_lru_node* replaced = page_table_lru.tail->prev;
        invalidate_page(replaced->p_table_index);
        replace_page_table(replaced);
    }
    ptln->p_table_index=page_table_index;
    
    //initialize entries//
    for(int i=0;i<128;i++)
    {
        (*p_table).entry_table[i].valid_bit = INVALID;
    }
    return ptln;
    // return p_table;
}

page_table_entry *get_page_entry(unsigned int block_number /*virtual address*/, PCB* temp_pcb, Proc_Access_Info* temp_pai) //page table walk
{
    // printf("get page entry called\n");
    page_table* outer = temp_pcb->page_dir_base_addr;
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
            temp_pai->num_main_memory_misses++;
        }
        temp_pai->num_main_memory_hits++;
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
            temp_pai->num_main_memory_misses++;
        }
        temp_pai->num_main_memory_hits++;
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
            temp_pai->num_main_memory_misses++;
        }
        temp_pai->num_main_memory_hits++;
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

        mm->p_tables[middle]->entry_table[middleindex].pageframe.p_table_index = temp->p_table_index;
        temp_pai->num_main_memory_misses++;
    }
    temp_pai->num_main_memory_hits++;

    inner = mm->p_tables[middle]->entry_table[middleindex].pageframe.p_table_index;
    if(block_number>>3 == 524279)
    {
        mm->p_tables[middle]->entry_table[middleindex].shared_bit=SHARED;
    }
    else mm->p_tables[middle]->entry_table[middleindex].shared_bit=UNSHARED;
    // Innermost level
    // Each entry in middle level points to a page table containing 128 entries each
    unsigned int frameindex = block_number % 128;
    page_table_entry* retval;

    //Go to the table whose adress was found (inner), get the required entry (frameindex), return its pointer
    if(mm->p_tables[inner]->entry_table[frameindex].valid_bit==INVALID)
    {
        //fetch block
        unsigned int index = frame_table_index;
        while(mm->f_table.entry_table[index]->valid_bit==VALID) //looking for INVALID entry to remove
        {
            index++;
        }
        // main_memory_block temp = get_disk_block(index, temp_pcb->pid);
        main_memory_block mm_block = *(main_memory_block*)malloc(sizeof(main_memory_block));
        second_chance_node* scn = (second_chance_node*)malloc(sizeof(second_chance_node));
        scn->data = &mm_block;
        scn->block_number = index;
        scn->prev = second_chance_fifo->head;
        scn->next = second_chance_fifo->head->next;
        second_chance_fifo->head->next = scn;
        scn->next->prev = scn;
        scn->second_chance_bit=1;
        mm->f_table.entry_table[index]->valid_bit=VALID;
        mm->f_table.entry_table[index]->frame_number=block_number;
        mm->f_table.entry_table[index]->pid=temp_pcb->pid;

        total_page_count++;
        temp_pcb->page_count++;
        temp_pai->num_main_memory_misses++;
        mm->f_table.entry_table[index]->page_number=block_number;
    }
    temp_pai->num_main_memory_hits++;
    retval = &(mm->p_tables[inner]->entry_table[frameindex]);

    return retval;
}

void page_table_free(page_table* p_table)
{
    // printf("invalidating page");
    for(int i=0;i<128;i++)
    {
        if(p_table->entry_table[i].valid_bit==VALID)
        {
            if(p_table->granularity==1)
            {
                invalidate_page(p_table->entry_table[i].pageframe.p_table_index);
                unsigned int temp = p_table->entry_table[i].pageframe.p_table_index;
                page_table_free(mm->p_tables[temp]);
            }
            p_table->entry_table[i].valid_bit==INVALID;
        }
    }
    free(p_table);
    return;
}