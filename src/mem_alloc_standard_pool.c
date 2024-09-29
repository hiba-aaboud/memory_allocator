#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "mem_alloc_types.h"
#include "mem_alloc_standard_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

/////////////////////////////////////////////////////////////////////////////

#ifdef STDPOOL_POLICY
/* Get the value provided by the Makefile */
std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
#else
std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
#endif

/////////////////////////////////////////////////////////////////////////////

void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{    /* TO BE IMPLEMENTED */
    p->start_addr = my_mmap(size);
    p->end_addr = (void *)((char *)p->start_addr + size);
    mem_std_free_block_t *initial_free_block = (mem_std_free_block_t *)p->start_addr;
    set_block_size(&initial_free_block->header, size - 2 * sizeof(mem_std_block_header_footer_t));
    set_block_free(&initial_free_block->header);
    
    mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)initial_free_block + get_block_size(&initial_free_block->header) + sizeof(mem_std_block_header_footer_t));
    *footer = initial_free_block->header;
    
    initial_free_block->prev = NULL;
    initial_free_block->next = NULL;
    p->first_free = initial_free_block;
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
}


//question 6
static void split_free_block(mem_std_free_block_t *block, size_t size) {
    size_t block_size = get_block_size(&block->header);
    size_t sizeRestante = block_size - size - 2 * sizeof(mem_std_block_header_footer_t);

    if (sizeRestante >= sizeof(mem_std_free_block_t)) {
        //new block
        mem_std_free_block_t *new_block = (mem_std_free_block_t *)((char *)block + size + 2 * sizeof(mem_std_block_header_footer_t));

        set_block_size(&new_block->header, sizeRestante);
        set_block_free(&new_block->header);

        mem_std_block_header_footer_t *new_footer = (mem_std_block_header_footer_t *)((char *)new_block + sizeRestante + sizeof(mem_std_block_header_footer_t));
        *new_footer = new_block->header;

        new_block->next = block->next; //free list
        new_block->prev = block;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        set_block_size(&block->header, size);
    }
}


void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size)
{      /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);

    // if (size % MEM_ALIGN != 0) {
    // size = ((size / MEM_ALIGN) + 1) * MEM_ALIGN;
    // }
    size_t req_size = size + 2 * sizeof(mem_std_block_header_footer_t);

    mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
    while (current != NULL) {
        if (is_block_free(&current->header) && get_block_size(&current->header) >= req_size) {
            split_free_block(current, size);
            set_block_used(&current->header);
             if (current->prev) {
                current->prev->next = current->next;
            } else {
                pool->first_free = current->next;
            }
            if (current->next) {
                current->next->prev = current->prev;
            }

         return (char *)current + sizeof(mem_std_block_header_footer_t);
        }
        current = current->next;
    }
    return NULL; //no block found
    
}
//question 7
static void coalesce_blocks(mem_pool_t *pool, mem_std_free_block_t *block) {

    mem_std_free_block_t *next_block = (mem_std_free_block_t *)((char *)block + get_block_size(&block->header) + 2 * sizeof(mem_std_block_header_footer_t));
    if ((char *)next_block < (char *)pool->end_addr && is_block_free(&next_block->header)) {

        size_t new_size = get_block_size(&block->header) + get_block_size(&next_block->header) + 2 * sizeof(mem_std_block_header_footer_t);
        set_block_size(&block->header, new_size);

        mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + new_size + sizeof(mem_std_block_header_footer_t));
        *footer = block->header;

        block->next = next_block->next;
        if (next_block->next) {
            next_block->next->prev = block; 
        }
    }
    mem_std_free_block_t *prev_block = block->prev;
    if (prev_block && is_block_free(&prev_block->header)) {
        size_t new_size = get_block_size(&prev_block->header) + get_block_size(&block->header) + 2 * sizeof(mem_std_block_header_footer_t);
        set_block_size(&prev_block->header, new_size);

        mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)prev_block + new_size + sizeof(mem_std_block_header_footer_t));
        *footer = prev_block->header;

        prev_block->next = block->next; 
        if (block->next) {
            block->next->prev = prev_block; 
        }
    }
}


void mem_free_standard_pool(mem_pool_t *pool, void *addr)
{/* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    if (addr == NULL) {
        return;
    }

    mem_std_free_block_t *block = (mem_std_free_block_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));

    set_block_free(&block->header);

    mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + get_block_size(&block->header) + sizeof(mem_std_block_header_footer_t));
    *footer = block->header;
    coalesce_blocks(pool, block);

    mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
    mem_std_free_block_t *prev = NULL;
    while (current != NULL && (char *)current < (char *)block) {
        prev = current;
        current = current->next; //gdb lmochkil hna
    }

    block->next = current;
    block->prev = prev;

    if (current != NULL) {
        current->prev = block;
    }
    
    if (prev != NULL) {
        prev->next = block;
    } else {
        pool->first_free = block;  
    }


    printf("addres dyal free block: %p, size: %zu, \n", block, get_block_size(&block->header));
}

size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    if (addr == NULL) return 0;

    // mem_std_block_header_footer_t *header = (mem_std_block_header_footer_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));
    // return get_block_size(header);
}
