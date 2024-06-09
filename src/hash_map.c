#include "hash_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_ADDR_SIZE 51200
#define MAX_DIR_PAGE MAX_ADDR_SIZE / HASH_MAP_DIR_BLOCK_SIZE

// 如果哈希表已经存在，则重新打开, 否则初始化
void hash_table_init(const char* filename, BufferPool* pool, off_t n_directory_blocks) {
    FileIOResult res = init_buffer_pool(filename, pool);
    if (res == FILE_OPEN) {  // 如果哈希表已经存在，则重新打开
        printf("Hash table already exists, reopen\n");
        return;
    }

    // 初始化哈希控制块
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);  //* 锁定控制块
    ctrl->max_size = n_directory_blocks * HASH_MAP_DIR_BLOCK_SIZE;
    ctrl->free_block_head = (n_directory_blocks + 1) * PAGE_SIZE;
    ctrl->n_directory_blocks = n_directory_blocks;
    release(pool, 0);  //* 释放控制块

    // 初始化目录块
    for (off_t i = 1; i <= n_directory_blocks; i++) {
        HashMapDirectoryBlock* dir_block =
            (HashMapDirectoryBlock*)get_page(pool, i * PAGE_SIZE);  //* 锁定目录块
        memset(dir_block->directory, 0, sizeof(dir_block->directory));
        release(pool, i * PAGE_SIZE);  //* 释放目录块
    }

    // 初始化空闲块链表
    for (off_t fb_i = n_directory_blocks + 1; fb_i < MAX_DIR_PAGE; fb_i++) {
        HashMapBlock* hash_block =
            (HashMapBlock*)get_page(pool, fb_i * PAGE_SIZE);  //* 锁定哈希块
        hash_block->next = (fb_i + 1) * PAGE_SIZE;
        hash_block->n_items = 0;
        release(pool, fb_i * PAGE_SIZE);  //* 释放哈希块
    }
}

// 关闭哈希表
void hash_table_close(BufferPool* pool) {
    close_buffer_pool(pool);
}

// ------------------------------------------------

// 从空闲块链表中分配一个块
off_t hash_table_alloc(BufferPool* pool) {
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);  //* 锁定
    if (ctrl->free_block_head == 0) {  // 如果没有空闲块
        fprintf(stderr, "No free block\n");
        assert(0);
    }
    off_t block_addr = ctrl->free_block_head;
    HashMapBlock* hash_block = (HashMapBlock*)get_page(pool, block_addr);  //* 锁定
    ctrl->free_block_head = hash_block->next;
    hash_block->next = 0;
    release(pool, block_addr);  //* 释放
    release(pool, 0);           //* 释放
    assert((block_addr & PAGE_MASK) == 0);
    return block_addr;
}

// 释放地址为addr的块到空闲块链表
void hash_table_free(BufferPool* pool, off_t addr) {
    assert((addr & PAGE_MASK) == 0);
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);  //* 锁定
    HashMapBlock* hash_block = (HashMapBlock*)get_page(pool, addr);       //* 锁定
    hash_block->next = ctrl->free_block_head;
    ctrl->free_block_head = addr;
    release(pool, addr);  //* 释放
    release(pool, 0);     //* 释放
}

// ------------------------------------------------

// 在size空闲空间集中, 链表末尾插入addr
void hash_table_insert(BufferPool* pool, short size, off_t addr) {
    HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(
        pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    off_t curAddr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];
    release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);

    // 如果目录项为空，则新建哈希块
    if (curAddr == 0) {
        curAddr = hash_table_alloc(pool);
        dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = curAddr;
        HashMapBlock* hash_block = (HashMapBlock*)get_page(pool, curAddr);  //* 锁定
        hash_block->table[0] = addr;
        hash_block->n_items = 1;
        release(pool, curAddr);  //* 释放
        return;
    }

    HashMapBlock* hash_block = (HashMapBlock*)get_page(pool, curAddr);  //* 锁定
    off_t tempAddr = hash_block->next;
    while (tempAddr != 0) {
        release(pool, curAddr);  //* 释放旧块
        curAddr = tempAddr;
        hash_block = (HashMapBlock*)get_page(pool, curAddr);  //* 锁定新块
        tempAddr = hash_block->next;
    }

    // 如果末尾哈希快已满, 则新建空哈希块
    if (hash_block->n_items == HASH_MAP_BLOCK_SIZE) {
        off_t newAddr = hash_table_alloc(pool);
        hash_block->next = newAddr;
        release(pool, curAddr);  //* 释放旧块
        curAddr = newAddr;
        hash_block = (HashMapBlock*)get_page(pool, curAddr);  //* 锁定新块
    }
    hash_block->table[hash_block->n_items] = addr;
    hash_block->n_items++;
    release(pool, curAddr);  //* 释放
    return;
}

// 弹出至少包含size空闲空间的addr, 如果没有则返回-1
off_t hash_table_pop_lower_bound(BufferPool* pool, short size) {
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
    off_t ctrl_max_size = ctrl->max_size;
    release(pool, 0);

    for (int i = size; i < ctrl_max_size; i++) {
        HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(
            pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);  //* 锁定目录块
        off_t block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);  //* 释放目录块

        if (block_addr != 0) {
            HashMapBlock* block = (HashMapBlock*)get_page(pool, block_addr);  //* 锁定哈希块
            off_t addr = block->table[block->n_items - 1];
            block->n_items--;
            release(pool, block_addr);  //* 释放哈希块

            if (block->n_items == 0) {  // 如果块为空, 则将块移至空闲链表
                dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE] = block->next;
                hash_table_free(pool, block_addr);
            }
            return addr;
        }
    }
    return -1;
}

// 从size的空闲空间集, 删除地址为addr的块的记录
void hash_table_pop(BufferPool* pool, short size, off_t addr) {
    HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(
        pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    off_t curAddr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];
    assert(curAddr != 0);
    release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);

    off_t befAddr = 0;
    while (curAddr != 0) {
        HashMapBlock* block = (HashMapBlock*)get_page(pool, curAddr);  //* 锁定
        for (int i = 0; i < block->n_items; i++) {
            if (block->table[i] == addr) {
                block->n_items--;  // 删除并前移
                for (int j = i; j < block->n_items; j++)
                    block->table[j] = block->table[j + 1];
                release(pool, curAddr);  //* 释放

                if (block->n_items == 0) {  // 如果块为空, 则将块移至空闲链表
                    if (befAddr == 0) {     // 连接到目录块
                        dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = block->next;
                        hash_table_free(pool, curAddr);
                    } else {  // 连接到先前块
                        HashMapBlock* befBlock =
                            (HashMapBlock*)get_page(pool, befAddr);  //* 锁定先前块
                        befBlock->next = block->next;
                        release(pool, befAddr);  //* 释放先前块
                        hash_table_free(pool, curAddr);
                    }
                }
                return;
            }
        }
        off_t nextAddr = block->next;
        release(pool, curAddr);  //* 释放
        befAddr = curAddr;
        curAddr = nextAddr;
    }

    fprintf(stderr, "hash_table_pop: addr not found\n");
    assert(0);
    return;
}

// 打印哈希块缓冲区
void print_hash_table(BufferPool* pool) {
    // 哈希控制块所在的页
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);  //* 锁定
    printf("----------HASH TABLE----------\n");
    for (int i = 0; i < ctrl->max_size; i++) {
        // 第i个哈希目录项所处的页
        HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(
            pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);  //* 锁定
        // 获取目录项
        off_t block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];

        if (block_addr != 0) {
            printf("目录项%d:", i);
            while (block_addr != 0) {
                // 获取当前哈希非空闲块
                HashMapBlock* block = (HashMapBlock*)get_page(pool, block_addr);  //* 锁定
                printf("  [块%ld]:", block_addr);
                printf("{");
                for (int j = 0; j < block->n_items; j++) {
                    if (j != 0)
                        printf(", ");
                    // 打印当前哈希非空闲块的所有空间地址
                    printf("%ld", block->table[j]);
                }
                printf("}");
                off_t next_addr = block->next;
                release(pool, block_addr);  //* 释放
                block_addr = next_addr;
            }
            printf("\n");
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);  //* 释放
    }
    release(pool, 0);  //* 释放
    printf("------------------------------\n\n");
}