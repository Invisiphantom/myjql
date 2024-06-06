#include "hash_map.h"

#include <stdio.h>

void hash_table_init(const char* filename, BufferPool* pool, off_t n_directory_blocks) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
}

void hash_table_close(BufferPool* pool) {
    close_buffer_pool(pool);
}

// 标记地址为addr的块有size的空闲空间
void hash_table_insert(BufferPool* pool, short size, off_t addr) {}

// 返回至少包含size空闲空间的块地址
off_t hash_table_pop_lower_bound(BufferPool* pool, short size) {}

// 删除地址为addr的块的记录，该块有size的空闲空间
void hash_table_pop(BufferPool* pool, short size, off_t addr) {}

// 打印哈希块缓冲区
void print_hash_table(BufferPool* pool) {
    // 哈希控制块所在的页
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0); //* 锁定
    printf("----------HASH TABLE----------\n");
    for (int i = 0; i < ctrl->max_size; i++) {
        // 第i个哈希目录块所处的页
        HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(
            pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE); //* 锁定
        // 获取该目录下的起始非空闲块页地址
        off_t block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];

        if (block_addr != 0) {
            printf("目录块%d:", i);
            while (block_addr != 0) {
                // 获取当前哈希非空闲块
                HashMapBlock* block = (HashMapBlock*)get_page(pool, block_addr); //* 锁定
                printf("  [" FORMAT_OFF_T "]", block_addr);
                printf("{");
                for (int j = 0; j < block->n_items; j++) {
                    if (j != 0)
                        printf(", ");
                    // 打印当前哈希非空闲块的所有空间地址
                    printf(FORMAT_OFF_T, block->table[j]);
                }
                printf("}");
                off_t next_addr = block->next;
                release(pool, block_addr); //* 释放
                block_addr = next_addr;
            }
            printf("\n");
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE); //* 释放
    }
    release(pool, 0); //* 释放
    printf("------------------------------\n");
}