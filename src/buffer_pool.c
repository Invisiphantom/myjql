#include "buffer_pool.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

// 打开filename，并关联pool为其缓冲池
FileIOResult init_buffer_pool(const char* filename, BufferPool* pool) {
    FileIOResult res = open_file(&pool->file, filename);
    // 初始化age为最大值, ref为0
    for (int i = 0; i < CACHE_PAGE; i++) {
        pool->addrs[i] = -1;
        pool->age[i] = SIZE_MAX;
        pool->ref[i] = 0;
    }
    return res;
}

// 关闭缓冲池，将缓冲的页写回文件
void close_buffer_pool(BufferPool* pool) {
    for (int i = 0; i < CACHE_PAGE; i++) {
        if (pool->addrs[i] != -1)
            write_page(&pool->pages[i], &pool->file, pool->addrs[i]);
    }
    close_file(&pool->file);
}

// 获取地址为addr的页，并锁定（保证该页不会被意外换出）
Page* get_page(BufferPool* pool, off_t addr) {
    for (int i = 0; i < CACHE_PAGE; i++)
        pool->age[i]++;  // LRU算法
    for (int i = 0; i < CACHE_PAGE; i++) {
        // 如果该页已经在缓冲池中, 则直接返回
        if (pool->addrs[i] == addr) {
            if (pool->ref[i] == 1) {
                fprintf(stderr, "get_page: page is locked\n");
                assert(0);
            }
            pool->age[i] = 0;
            pool->ref[i]++;
            return &pool->pages[i];
        }
    }

    // 获取LRU页
    int swi_idx = 0;
    while (pool->ref[swi_idx] != 0)
        swi_idx++;

    for (int i = 0; i < CACHE_PAGE; i++) {
        if (pool->ref[i] == 0 && pool->age[i] > pool->age[swi_idx]) {
            swi_idx = i;
        }
    }

    // 如果该页非空闲, 则写回文件
    if (pool->addrs[swi_idx] != -1) {
        if (pool->ref[swi_idx] != 0) {
            fprintf(stderr, "Error: page is locked\n");
            assert(0);
        }
        write_page(&pool->pages[swi_idx], &pool->file, pool->addrs[swi_idx]);
    }

    // 如果文件中没有该页, 则扩充文件
    read_page(&pool->pages[swi_idx], &pool->file, addr);
    pool->addrs[swi_idx] = addr;
    pool->age[swi_idx] = 0;
    pool->ref[swi_idx]++;
    return &pool->pages[swi_idx];
}

// 释放地址为addr的页，该页之后可以被换出
void release(BufferPool* pool, off_t addr) {
    for (int i = 0; i < CACHE_PAGE; i++) {
        if (pool->addrs[i] == addr) {
            if (pool->ref[i] == 0) {
                fprintf(stderr, "release: ref=0 can't --\n");
                assert(0);
            }
            pool->ref[i]--;
            return;
        }
    }
    fprintf(stderr, "Error: page not found\n");
    exit(1);
}

/* void print_buffer_pool(BufferPool *pool) {
} */

/* void validate_buffer_pool(BufferPool *pool) {
} */
