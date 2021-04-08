#include <unistd.h>
#include <stdlib.h>
#include "pagetable.h"

page_table* page_table_initialize()
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

void page_table_free(page_table* p_table)
{
    free(p_table);
    return;
}