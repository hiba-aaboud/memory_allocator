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
{//best fit
    mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
    mem_std_free_block_t *best_fit = NULL;
    size_t smallest_waste = SIZE_MAX;

    //best fit block
    while (current != NULL)
    {
        if (is_block_free(&current->header) && get_block_size(&current->header) >= size)
        {
            size_t full_size = get_block_size(&current->header);
            size_t waste = full_size - size;

            if (waste < smallest_waste) 
            {
                smallest_waste = waste;
                best_fit = current;
            }
        }
        current = current->next; 
    }

    if (best_fit != NULL)
    {
        size_t full_size = get_block_size(&best_fit->header);
        set_block_size(&best_fit->header, size);
        set_block_used(&best_fit->header);

        size_t remaining_size = full_size - size - 2 * sizeof(mem_std_block_header_footer_t);
        printf("Best fit block size is %zu, allocated size: %zu\n", full_size, size);

        if (remaining_size >= pool->min_req_size)
        {
            printf("Splitting block, remaining size: %zu\n", remaining_size);
            mem_std_free_block_t *new_block = (mem_std_free_block_t *)((char *)best_fit + size + 2 * sizeof(mem_std_block_header_footer_t));
            set_block_size(&new_block->header, remaining_size);
            set_block_free(&new_block->header);

            // Adjust the free list
            new_block->next = best_fit->next;
            new_block->prev = best_fit->prev;

            if (best_fit->prev)
            {
                best_fit->prev->next = new_block;
            }
            else
            {
                pool->first_free = new_block;
            }

            if (best_fit->next)
            {
                best_fit->next->prev = new_block;
            }
        }
        else
        {
            if (best_fit->prev)
            {
                best_fit->prev->next = best_fit->next;
            }
            else
            {
                pool->first_free = best_fit->next;
            }

            if (best_fit->next)
            {
                best_fit->next->prev = best_fit->prev;
            }
        }

        return (char *)best_fit + sizeof(mem_std_block_header_footer_t); // Return the allocated block address
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

// void mem_free_standard_pool(mem_pool_t *pool, void *addr)
// {
//     if (addr == NULL)
//     {
//         return;
//     }

//     mem_std_free_block_t *block = (mem_std_free_block_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));

//     set_block_free(&block->header);

//     mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + get_block_size(&block->header) + sizeof(mem_std_block_header_footer_t));

//     if ((char *)footer >= (char *)pool->end_addr)
//     {
//         printf("Error: Footer is out of bounds!\n");
//         return;
//     }

//     *footer = block->header;

//     mem_std_free_block_t *current = pool->first_free;
//     mem_std_free_block_t *prev = NULL;

//     while (current != NULL && (char *)current < (char *)block)
//     {
//         prev = current;
//         current = current->next;
//     }

//     block->next = current;
//     block->prev = prev;

//     if (current != NULL)
//     {
//         current->prev = block;
//     }
//     if (prev != NULL)
//     {
//         prev->next = block;
//     }
//     else
//     {
//         pool->first_free = block;
//     }

//     printf("Freed block at: %lu, size: %zu\n", ((char *)block - (char *)pool->start_addr), get_block_size(&block->header));

//     // Coalesce adjacent free blocks
//     current = pool->first_free;
//     while (current)
//     {
//         if ((char *)current + get_block_size(&current->header) + 2 * sizeof(mem_std_block_header_footer_t) == (char *)current->next)
//         {
//             set_block_size(&current->header, get_block_size(&current->header) + get_block_size(&current->next->header) + 2 * sizeof(mem_std_block_header_footer_t));
//             current->next = current->next->next;
//         }
//         printf("Free block at %lu, size: %zu\n", (char *)current - (char *)pool->start_addr, get_block_size(&current->header));

//         current = current->next;
//     }
// }


size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr)
{
    mem_std_allocated_block_t* block = (mem_std_allocated_block_t*)addr; 
    size_t size = get_block_size(&block->header);
    printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
    return size ;
}






/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// #include <stdlib.h>
// #include <assert.h>
// #include <stdio.h>

// #include "mem_alloc_types.h"
// #include "mem_alloc_standard_pool.h"
// #include "my_mmap.h"
// #include "mem_alloc.h"

// /////////////////////////////////////////////////////////////////////////////

// #ifdef STDPOOL_POLICY
// /* Get the value provided by the Makefile */
// std_pool_placement_policy_t std_pool_policy = STDPOOL_POLICY;
// #else
// std_pool_placement_policy_t std_pool_policy = DEFAULT_STDPOOL_POLICY;
// #endif

// #define MIN_FREE_BLOCK_SIZE (sizeof(mem_std_free_block_t))  

// /////////////////////////////////////////////////////////////////////////////

// void init_standard_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size) {
//     /* Initialize the memory pool */
//     p->start_addr = my_mmap(size);
//     p->end_addr = (void *)((char *)p->start_addr + size);
//     p->min_req_size = min_request_size;
//     p->max_req_size = max_request_size;

//     // The first free block should start at the beginning of the pool
//     mem_std_free_block_t *initial_free_block = (mem_std_free_block_t *)p->start_addr;
    
//     // Set block size: full pool size minus two header/footer sizes
//     set_block_size(&initial_free_block->header, size - 2 * sizeof(mem_std_block_header_footer_t));
//     set_block_free(&initial_free_block->header);

//     // Set the footer for the initial free block
//     mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)initial_free_block + get_block_size(&initial_free_block->header) + sizeof(mem_std_block_header_footer_t));
//     *footer = initial_free_block->header;

//     initial_free_block->prev = NULL;
//     initial_free_block->next = NULL;
//     p->first_free = initial_free_block;

//     printf("Pool initialized with a free block starting at 0x%p, size: %zu\n", initial_free_block, get_block_size(&initial_free_block->header));
// }



// /////////////////////////////////////////////////////////////////////////////

// static void split_free_block(mem_pool_t *pool, mem_std_free_block_t *block, size_t size) {
//     size_t block_size = get_block_size(&block->header);
//     size_t remaining_size = block_size - size - 2 * sizeof(mem_std_block_header_footer_t);

//     // Only split if the remaining size is big enough to hold a free block
//     if (remaining_size >= pool->min_req_size) {
//         // Calculate new block's position
//         mem_std_free_block_t *new_block = (mem_std_free_block_t *)((char *)block + size + 2 * sizeof(mem_std_block_header_footer_t));

//         // Boundary check for new block
//         if ((char *)new_block >= (char *)pool->end_addr) {
//             printf("Error: New block exceeds pool boundary!\n");
//             return;
//         }

//         // Set block size and mark it as free
//         set_block_size(&new_block->header, remaining_size);
//         set_block_free(&new_block->header);

//         // Calculate the new footer for the new block
//         mem_std_block_header_footer_t *new_footer = (mem_std_block_header_footer_t *)((char *)new_block + remaining_size + sizeof(mem_std_block_header_footer_t));

//         // Boundary check for new footer
//         if ((char *)new_footer >= (char *)pool->end_addr) {
//             printf("Error: new_footer exceeds pool boundary!\n");
//             return;
//         }

//         *new_footer = new_block->header;

//         // Update the free list
//         new_block->next = block->next;
//         new_block->prev = block;
//         if (block->next) {
//             block->next->prev = new_block;
//         }
//         block->next = new_block;

//         // Update the size of the current block
//         set_block_size(&block->header, size);
//     }
// }


// /////////////////////////////////////////////////////////////////////////////

// void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) {
//     printf("%s:%d: Allocating memory of size: %zu\n", __FUNCTION__, __LINE__, size);

//     size_t req_size = size + 2 * sizeof(mem_std_block_header_footer_t);

  
//     mem_std_free_block_t *current = (mem_std_free_block_t *)pool->first_free;
//     while (current != NULL) {
//         if (is_block_free(&current->header) && get_block_size(&current->header) >= size) {
        
//             set_block_used(&current->header);
//             split_free_block(pool, current, size);


//             if (current->prev) {
//                 current->prev->next = current->next;
//             } else {
//                 pool->first_free = current->next;
//             }
//             if (current->next) {
//                 current->next->prev = current->prev;
//             }

//             return (char *)current + sizeof(mem_std_block_header_footer_t);
//         }
//         current = current->next;
//     }

//     // No suitable block found
//     return NULL;
// }

// /////////////////////////////////////////////////////////////////////////////
// void coalesce_blocks(mem_pool_t *pool, mem_std_free_block_t *block) {
//     // Coalesce with the next block if it is free
//     mem_std_free_block_t *next_block = (mem_std_free_block_t *)((char *)block + get_block_size(&block->header) + 2 * sizeof(mem_std_block_header_footer_t));

//     if ((char *)next_block < (char *)pool->end_addr && is_block_free(&next_block->header)) {
//         size_t next_block_size = get_block_size(&next_block->header);
//         size_t new_size = get_block_size(&block->header) + next_block_size + 2 * sizeof(mem_std_block_header_footer_t);

//         // Ensure new coalesced block fits within the pool
//         if ((char *)block + new_size + sizeof(mem_std_block_header_footer_t) > (char *)pool->end_addr) {
//             printf("Error: Coalesced block exceeds pool boundary!\n");
//             return;
//         }

//         // Set the new size for the coalesced block
//         set_block_size(&block->header, new_size);

//         // Update the footer for the coalesced block
//         mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + new_size + sizeof(mem_std_block_header_footer_t));
        
//         // Boundary check for footer
//         if ((char *)footer >= (char *)pool->end_addr) {
//             printf("Error: Footer exceeds pool boundary!\n");
//             return;
//         }

//         *footer = block->header;

//         // Update the free list
//         block->next = next_block->next;
//         if (next_block->next) {
//             next_block->next->prev = block;
//         }

//         printf("Coalesced with next block at %p, new size: %zu\n", block, get_block_size(&block->header));
//     }

//     // Coalesce with the previous block if it is free
//     mem_std_free_block_t *prev_block = block->prev;
//     if (prev_block && is_block_free(&prev_block->header)) {
//         size_t prev_block_size = get_block_size(&prev_block->header);
//         size_t new_size = prev_block_size + get_block_size(&block->header) + 2 * sizeof(mem_std_block_header_footer_t);

//         // Ensure new coalesced block fits within the pool
//         if ((char *)prev_block + new_size + sizeof(mem_std_block_header_footer_t) > (char *)pool->end_addr) {
//             printf("Error: Coalesced block exceeds pool boundary!\n");
//             return;
//         }

//         // Set the new size for the coalesced block
//         set_block_size(&prev_block->header, new_size);

//         // Update the footer for the coalesced block
//         mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)prev_block + new_size + sizeof(mem_std_block_header_footer_t));

//         // Boundary check for footer
//         if ((char *)footer >= (char *)pool->end_addr) {
//             printf("Error: Footer exceeds pool boundary!\n");
//             return;
//         }

//         *footer = prev_block->header;

//         // Update the free list
//         prev_block->next = block->next;
//         if (block->next) {
//             block->next->prev = prev_block;
//         }

//         printf("Coalesced with previous block at %p, new size: %zu\n", prev_block, get_block_size(&prev_block->header));
//     }
// }


// /////////////////////////////////////////////////////////////////////////////

// void mem_free_standard_pool(mem_pool_t *pool, void *addr) {
//     if (addr == NULL) {
//         return;
//     }

//     // Calculate the start of the block (header) from the address
//     mem_std_free_block_t *block = (mem_std_free_block_t *)((char *)addr - sizeof(mem_std_block_header_footer_t));

//     // Mark the block as free
//     set_block_free(&block->header);

//     // Calculate footer position and check boundary
//     mem_std_block_header_footer_t *footer = (mem_std_block_header_footer_t *)((char *)block + get_block_size(&block->header) + sizeof(mem_std_block_header_footer_t));
//     if ((char *)footer >= (char *)pool->end_addr) {
//         printf("Error: Footer is out of bounds!\n");
//         return;
//     }

//     *footer = block->header;  // Set the footer to match the header

//     // Attempt to coalesce with adjacent blocks
//     coalesce_blocks(pool, block);

//     // Insert the block back into the free list in the right position
//     mem_std_free_block_t *current = pool->first_free;
//     mem_std_free_block_t *prev = NULL;

//     // Traverse the free list to find the correct position
//     while (current != NULL && (char *)current < (char *)block) {
//         prev = current;
//         current = current->next;
//     }

//     block->next = current;
//     block->prev = prev;

//     if (current != NULL) {
//         current->prev = block;
//     }
//     if (prev != NULL) {
//         prev->next = block;
//     } else {
//         pool->first_free = block;
//     }

//     printf("Freed block at: %p, size: %zu\n", block, get_block_size(&block->header));
//     printf("Free list:\n");
    
//     // Print the current state of the free list for debugging
//     current = pool->first_free;
//     while (current) {
//         printf("Free block at %p, size: %zu\n", current, get_block_size(&current->header));
//         current = current->next;
//     }
// }


// /////////////////////////////////////////////////////////////////////////////

// size_t mem_get_allocated_block_size_standard_pool(mem_pool_t *pool, void *addr) {
//     printf("%s:%d: Please, implement me!\n", __FUNCTION__, __LINE__);
//     return 0;
// }



// // static mem_std_free_block_t *last_allocated = NULL;

// // void *mem_alloc_standard_pool(mem_pool_t *pool, size_t size) { //next fit
// //     printf("%s:%d: Implementing Next Fit policy.\n", __FUNCTION__, __LINE__);

// //     size_t req_size = size + 2 * sizeof(mem_std_block_header_footer_t);
// //     mem_std_free_block_t *current = last_allocated ? last_allocated->next : (mem_std_free_block_t *)pool->first_free;

// //     // Search from last allocated block onward
// //     while (current != NULL) {
// //         if (is_block_free(&current->header) && get_block_size(&current->header) >= req_size) {
// //             // Found a suitable block, split and mark it as used
// //             split_free_block(current, size);
// //             set_block_used(&current->header);

// //             // Remove the block from the free list
// //             if (current->prev) {
// //                 current->prev->next = current->next;
// //             } else {
// //                 pool->first_free = current->next;
// //             }
// //             if (current->next) {
// //                 current->next->prev = current->prev;
// //             }

// //             last_allocated = current;  // Update last_allocated
// //             return (char *)current + sizeof(mem_std_block_header_footer_t); // Return usable memory
// //         }
// //         current = current->next;
// //     }

// //     // If no block was found after last_allocated, try searching from the beginning
// //     current = (mem_std_free_block_t *)pool->first_free;
// //     while (current != last_allocated) {
// //         if (is_block_free(&current->header) && get_block_size(&current->header) >= req_size) {
// //             // Found a suitable block, split and mark it as used
// //             split_free_block(current, size);
// //             set_block_used(&current->header);

// //             // Remove the block from the free list
// //             if (current->prev) {
// //                 current->prev->next = current->next;
// //             } else {
// //                 pool->first_free = current->next;
// //             }
// //             if (current->next) {
// //                 current->next->prev = current->prev;
// //             }

// //             last_allocated = current;  // Update last_allocated
// //             return (char *)current + sizeof(mem_std_block_header_footer_t); // Return usable memory
// //         }
// //         current = current->next;
// //     }

// //     // No suitable block found
// //     return NULL;
// // }

