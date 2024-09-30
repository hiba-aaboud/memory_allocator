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
#define MIN_FREE_BLOCK_SIZE (sizeof(mem_std_free_block_t))  

/////////////////////////////////////////////////////////////////////////////

void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{    /* TO BE IMPLEMENTED */
    p->start_addr = my_mmap(size);
    p->end_addr = (void *)((char *)p->start_addr + size);
    p->min_req_size = min_request_size;
    p->max_req_size = max_request_size;
    mem_std_free_block_t *initial_free_block = (mem_std_free_block_t *)p->start_addr;
    set_block_size(&initial_free_block->header, size - 2 * sizeof(mem_std_block_header_footer_t));
    set_block_free(&initial_free_block->header);
    
    mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)initial_free_block + get_block_size(&initial_free_block->header) + sizeof(mem_std_block_header_footer_t));
    *footer = initial_free_block->header;
    
    initial_free_block->prev = NULL;
    initial_free_block->next = NULL;
    p->first_free = initial_free_block;

}


//question 6
static void split_free_block(mem_std_free_block_t *block, size_t size) {
    size_t block_size = get_block_size(&block->header);
    size_t remaining_size = block_size - size - 2 * sizeof(mem_std_block_header_footer_t);

    if (remaining_size >= MIN_FREE_BLOCK_SIZE) {

        mem_std_free_block_t *new_block = (mem_std_free_block_t *)((char *)block + size + 2 * sizeof(mem_std_block_header_footer_t));

        set_block_size(&new_block->header, remaining_size);
        set_block_free(&new_block->header);

        mem_std_block_header_footer_t *new_footer = (mem_std_block_header_footer_t *)((char *)new_block + remaining_size + sizeof(mem_std_block_header_footer_t));
        *new_footer = new_block->header;
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;

        set_block_size(&block->header, size);
    } else {
        set_block_size(&block->header, block_size);
    }
}


void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size)
{      /* TO BE IMPLEMENTED */ //first fit
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);

    // if (size % MEM_ALIGN != 0) {
    // size = ((size / MEM_ALIGN) + 1) * MEM_ALIGN;
    // }
    size_t req_size = size + 2 * sizeof(mem_std_block_header_footer_t);

    //first fit
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

    //no block found
    return NULL;
}
//question 7
void coalesce_blocks(mem_pool_t *pool, mem_std_free_block_t *block) {
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

void mem_free_standard_pool(mem_pool_t *pool, void *addr) {
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
        current = current->next;
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
}





size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    /* TO BE IMPLEMENTED */
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    if (addr == NULL) return 0;

    // mem_std_block_header_footer_t *header = (mem_std_block_header_footer_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));
    // return get_block_size(header);
}


// static mem_std_free_block_t *last_allocated = NULL;

// void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) { //next fit
//     printf("%s:%d: Implementing Next Fit policy.\n", __FUNCTION__, __LINE__);

//     size_t req_size = size + 2 * sizeof(mem_std_block_header_footer_t);
//     mem_std_free_block_t *current = last_allocated ? last_allocated->next : (mem_std_free_block_t *)pool->first_free;

//     // Search from last allocated block onward
//     while (current != NULL) {
//         if (is_block_free(&current->header) && get_block_size(&current->header) >= req_size) {
//             // Found a suitable block, split and mark it as used
//             split_free_block(current, size);
//             set_block_used(&current->header);

//             // Remove the block from the free list
//             if (current->prev) {
//                 current->prev->next = current->next;
//             } else {
//                 pool->first_free = current->next;
//             }
//             if (current->next) {
//                 current->next->prev = current->prev;
//             }

//             last_allocated = current;  // Update last_allocated
//             return (char *)current + sizeof(mem_std_block_header_footer_t); // Return usable memory
//         }
//         current = current->next;
//     }

//     // If no block was found after last_allocated, try searching from the beginning
//     current = (mem_std_free_block_t *)pool->first_free;
//     while (current != last_allocated) {
//         if (is_block_free(&current->header) && get_block_size(&current->header) >= req_size) {
//             // Found a suitable block, split and mark it as used
//             split_free_block(current, size);
//             set_block_used(&current->header);

//             // Remove the block from the free list
//             if (current->prev) {
//                 current->prev->next = current->next;
//             } else {
//                 pool->first_free = current->next;
//             }
//             if (current->next) {
//                 current->next->prev = current->prev;
//             }

//             last_allocated = current;  // Update last_allocated
//             return (char *)current + sizeof(mem_std_block_header_footer_t); // Return usable memory
//         }
//         current = current->next;
//     }

//     // No suitable block found
//     return NULL;
// }

// void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) { //best fit
//     printf("%s:%d: Implementing Best Fit policy.\n", __FUNCTION__, __LINE__);

//     size_t req_size = size + 2 * sizeof(mem_std_block_header_footer_t);
//     mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
//     mem_std_free_block_t *best_fit = NULL;
//     size_t smallest_waste = SIZE_MAX;

//     while (current != NULL) {
//         size_t block_size = get_block_size(&current->header);
//         if (is_block_free(&current->header) && block_size >= req_size) {
//             size_t waste = block_size - req_size;
//             if (waste < smallest_waste) {
//                 smallest_waste = waste;
//                 best_fit = current;
//             }
//         }
//         current = current->next;
//     }

//     if (best_fit != NULL) {
//         // Found the best fit block, split and mark it as used
//         split_free_block(best_fit, size);
//         set_block_used(&best_fit->header);

//         // Remove the block from the free list
//         if (best_fit->prev) {
//             best_fit->prev->next = best_fit->next;
//         } else {
//             pool->first_free = best_fit->next;
//         }
//         if (best_fit->next) {
//             best_fit->next->prev = best_fit->prev;
//         }

//         return (char *)best_fit + sizeof(mem_std_block_header_footer_t); // Return usable memory
//     }

//     // No suitable block found
//     return NULL;
// }
