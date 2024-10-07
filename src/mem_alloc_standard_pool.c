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
{
   /* TO BE IMPLEMENTED */
    p->start_addr = my_mmap(size);
    p->end_addr = (void *)((char *)p->start_addr + size);
    p->min_req_size = min_request_size;
    p->max_req_size = max_request_size;

    mem_std_free_block_t *first_free = (mem_std_free_block_t *)((char *)p->start_addr);

    set_block_size(&first_free->header, size - 2 * sizeof(mem_std_block_header_footer_t));
    set_block_free(&first_free->header);

    first_free->next = NULL;
    first_free->prev = NULL;

    p->first_free = first_free;
}

void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size)
{

    mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
    while (current != NULL)
    {
        if (is_block_free(&current->header) && get_block_size(&current->header) >= size)
        {
            size_t full_size = get_block_size(&current->header);
            set_block_size(&current->header, size);
            set_block_used(&current->header);

            size_t remaining_size = full_size - size - 2 * sizeof(mem_std_block_header_footer_t);
            printf("block size is %zu, allocated size: %zu\n ", full_size, size);
            if (full_size > size)
            {
                printf("lets split and rem size is : %zu\n", remaining_size);
                mem_std_free_block_t *new = (mem_std_free_block_t *)((char *)current + size + 2 * sizeof(mem_std_block_header_footer_t));
                set_block_size(&new->header, remaining_size);
                set_block_free(&new->header);
                if (current->prev)
                {
                    current->prev->next = new;
                    new->prev = current->prev;
                }
                else
                    pool->first_free = new;

                if (current->next)
                {
                    new->next = current->next;
                    current->next->prev = new;
                }
            }
            else
            {
                if (current->prev)
                {
                    current->prev->next = current->next;
                }
                else
                {
                    pool->first_free = current->next;
                }
                if (current->next)
                {
                    current->next->prev = current->prev;
                }
            }

            return (char *)current + sizeof(mem_std_block_header_footer_t);
        }
        current = current->next;
    }

    return NULL;
}

void mem_free_standard_pool(mem_pool_t *pool, void *addr)
{
    if (addr == NULL)
    {
        return;
    }

    mem_std_free_block_t *block = (mem_std_free_block_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));

    set_block_free(&block->header);

    mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + get_block_size(&block->header) + sizeof(mem_std_block_header_footer_t));
    printf("block to be freed: %lu, size: %zu\n", ((char *)block - (char *)pool->start_addr), get_block_size(&block->header));

    if ((char *)footer >= (char *)pool->end_addr)
    {
        printf("Error: Footer is out of bounds!\n");
        return;
    }

    *footer = block->header;

    mem_std_free_block_t *current = pool->first_free;
    mem_std_free_block_t *prev = NULL;

    while (current != NULL && (char *)current < (char *)block)
    {
        prev = current;
        current = current->next;
    }

    block->next = current;
    block->prev = prev;

    if (current != NULL)
    {
        current->prev = block;
    }
    if (prev != NULL)
    {
        prev->next = block;
    }
    else
    {
        pool->first_free = block;
    }

    printf("Freed block at: %lu, size: %zu\n", ((char *)block - (char *)pool->start_addr), get_block_size(&block->header));
    
    printf("Free list:\n");

    current = (mem_std_free_block_t *)pool->first_free;
    while (current)
    {
        if ((char *)current + get_block_size(&current->header) + 2 * sizeof(mem_std_block_header_footer_t) == (char *)current->next)
        {
            set_block_size(&current->header, get_block_size(&current->header) + get_block_size(&current->next->header) + 2 * sizeof(mem_std_block_header_footer_t));
            current->next = current->next->next;
        }
        printf("Free block at %lu, size: %zu\n", (char *)current - (char *)pool->start_addr, get_block_size(&current->header));

        current = current->next;
    }
}

size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    mem_std_allocated_block_t* block = (mem_std_allocated_block_t*)addr; 
    size_t size = get_block_size(&block->header);
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    return size ;
}
