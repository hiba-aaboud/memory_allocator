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



// void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size)
// {//first fit

//     mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
//     while (current != NULL)
//     {
//         if (is_block_free(&current->header) && get_block_size(&current->header) >= size)
//         {
//             size_t full_size = get_block_size(&current->header);
//             set_block_size(&current->header, size);
//             set_block_used(&current->header);

//             size_t remaining_size = full_size - size - 2 * sizeof(mem_std_block_header_footer_t);
//             printf("block size is %zu, allocated size: %zu\n ", full_size, size);
//             if (full_size > size)
//             {
//                 printf("lets split and rem size is : %zu\n", remaining_size);
//                 mem_std_free_block_t *new = (mem_std_free_block_t *)((char *)current + size + 2 * sizeof(mem_std_block_header_footer_t));
//                 set_block_size(&new->header, remaining_size);
//                 set_block_free(&new->header);
//                 if (current->prev)
//                 {
//                     current->prev->next = new;
//                     new->prev = current->prev;
//                 }
//                 else
//                     pool->first_free = new;

//                 if (current->next)
//                 {
//                     new->next = current->next;
//                     current->next->prev = new;
//                 }
//             }
//             else
//             {
//                 if (current->prev)
//                 {
//                     current->prev->next = current->next;
//                 }
//                 else
//                 {
//                     pool->first_free = current->next;
//                 }
//                 if (current->next)
//                 {
//                     current->next->prev = current->prev;
//                 }
//             }

//             return (char *)current + sizeof(mem_std_block_header_footer_t);
//         }
//         current = current->next;
//     }

//     return NULL;
// }


void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size)
{
    mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
    mem_std_free_block_t *best_fit = NULL;  //for bf
    static mem_std_free_block_t *last_allocated = NULL;  //for nf
    size_t smallest_waste = SIZE_MAX;

  
    switch (std_pool_policy) {
        case FIRST_FIT:
            while (current != NULL) {
                if (is_block_free(&current->header) && get_block_size(&current->header) >= size) {
                    break;  
                }
                current = current->next;
            }
            break;

        case BEST_FIT:
  
            while (current != NULL) {
                if (is_block_free(&current->header) && get_block_size(&current->header) >= size) {
                    size_t full_size = get_block_size(&current->header);
                    size_t waste = full_size - size;
                    if (waste < smallest_waste) {
                        smallest_waste = waste;
                        best_fit = current;
                    }
                }
                current = current->next;
            }
            current = best_fit;  
            break;

        case NEXT_FIT:
            if (last_allocated == NULL) {
                current = pool->first_free; 
            } else {
                if (last_allocated->next != NULL) {
    current = last_allocated->next; 
} else {
    current = pool->first_free; 
}

            }

            while (current != NULL) {
                if (is_block_free(&current->header) && get_block_size(&current->header) >= size) {
                    last_allocated = current;
                    break;
                }
                current = current->next;
            }

            if (current == NULL) {
                current = pool->first_free;
                while (current != last_allocated) {
                    if (is_block_free(&current->header) && get_block_size(&current->header) >= size) {
                        last_allocated = current;
                        break;
                    }
                    current = current->next;
                }
            }
            break;

        default:
            printf("Error:Invalid memory allocation policy\n");
            return NULL;
    }

    if (current != NULL) {
        size_t full_size = get_block_size(&current->header);
        set_block_size(&current->header, size);
        set_block_used(&current->header);

        size_t remaining_size = full_size - size - 2 * sizeof(mem_std_block_header_footer_t);
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

    // No suitable block found
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

    current = pool->first_free;
    while (current)
    {
        if ((char *)current + get_block_size(&current->header) + 2 * sizeof(mem_std_block_header_footer_t) == (char *)current->next)
        {
            //merge the blocks
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






