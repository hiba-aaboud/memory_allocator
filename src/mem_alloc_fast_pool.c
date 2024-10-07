#include <assert.h>
#include <stdio.h>
#include "mem_alloc_fast_pool.h"
#include "my_mmap.h"
#include "mem_alloc.h"

void init_fast_pool(mem_pool_t *p, size_t size, size_t min_request_size, size_t max_request_size)
{
    p->start_addr = my_mmap(size);
    p->end_addr = (void *)((char *)p->start_addr + size);
    p->first_free = p->start_addr;
    p->min_req_size = min_request_size;
    p->max_req_size = max_request_size;

    mem_fast_free_block_t *current_block = (mem_fast_free_block_t *)p->start_addr;
    mem_fast_free_block_t *next_block = NULL;

    while ((void *)((char *)current_block + max_request_size) < p->end_addr)
    {
        next_block = (mem_fast_free_block_t *)((char *)current_block + max_request_size);
        current_block->next = next_block;
        current_block = next_block;
    }
    current_block->next = NULL;

    printf("%s:%d:Init implemented!\n", __FUNCTION__, __LINE__);
}

void *mem_alloc_fast_pool(mem_pool_t *pool, size_t size)
{
    if (size > pool->max_req_size || size < pool->min_req_size)
    {
        exit(0);
    }

    if (pool->first_free == NULL)
    {
        printf("no mem");
        return NULL;
    }

    void *allocated_block = pool->first_free;
    mem_fast_free_block_t *first_b = (mem_fast_free_block_t *)pool->first_free;
    pool->first_free = first_b->next;

    printf("%s:%d: Alloc implemented!\n", __FUNCTION__, __LINE__);

    return allocated_block;
}

void mem_free_fast_pool(mem_pool_t *pool, void *b)
{
    if (b == NULL)
    {
        return;
    }

    mem_fast_free_block_t *freed_block = (mem_fast_free_block_t *)b;
    freed_block->next = pool->first_free;
    pool->first_free = freed_block;
}

size_t mem_get_allocated_block_size_fast_pool(mem_pool_t *pool, void *addr)
{
    size_t res;
    res = pool->max_req_size;
    return res;
}
