#ifndef _HASH_MAP_H
#define _HASH_MAP_H

#include "buffer_pool.h"

// 哈希控制块
typedef struct {
    off_t max_size;            // 最大目录项数
    off_t free_block_head;     // 空闲块链表头指针
    off_t n_directory_blocks;  // 第1~n_directory_blocks块是哈希目录块
} HashMapControlBlock;

// 目录页块可存放的目录项数
#define HASH_MAP_DIR_BLOCK_SIZE (PAGE_SIZE / sizeof(off_t))

// 哈希目录页块
typedef struct {
    off_t directory[HASH_MAP_DIR_BLOCK_SIZE];  // 工作链表头指针
} HashMapDirectoryBlock;

// 哈希映射块可存放的地址项数 (去掉next和n_items)
#define HASH_MAP_BLOCK_SIZE ((PAGE_SIZE - 2 * sizeof(off_t)) / sizeof(off_t))

// 哈希映射块
typedef struct {
    off_t next;     // 下一个块的地址
    off_t n_items;  // 该块中的空闲空间数目
    off_t table[HASH_MAP_BLOCK_SIZE];
} HashMapBlock;

// 如果哈希表已经存在，则重新打开, 否则初始化
void hash_table_init(const char* filename, BufferPool* pool, off_t n_directory_blocks);

// 关闭哈希表
void hash_table_close(BufferPool* pool);

// 从空闲块链表中分配一个块
off_t hash_table_alloc(BufferPool* pool);

// 释放地址为addr的块到空闲块链表
void hash_table_free(BufferPool* pool, off_t addr);

// 标记地址为addr的块有size的空闲空间
void hash_table_insert(BufferPool* pool, short size, off_t addr);

// 弹出至少包含size空闲空间的块地址, 如果没有则返回-1
off_t hash_table_pop_lower_bound(BufferPool* pool, short size);

// 删除地址为addr的块的记录，该块有size的空闲空间
void hash_table_pop(BufferPool* pool, short size, off_t addr);

// 打印哈希块缓冲区
void print_hash_table(BufferPool* pool);

#endif /* _HASH_MAP_H */