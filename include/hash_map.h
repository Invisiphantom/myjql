#ifndef _HASH_MAP_H
#define _HASH_MAP_H

#include "buffer_pool.h"

// .fsm文件 Free Space Map 空闲空间映射

// 哈希控制块(空闲链表)
typedef struct {
    off_t free_block_head;     // 空闲块链表头指针
    off_t n_directory_blocks;  // 第1~n_directory_blocks块是哈希目录块
    off_t max_size;
} HashMapControlBlock;

// 一页可存放的哈希目录块数目  sizeof(off_t)=8字节
#define HASH_MAP_DIR_BLOCK_SIZE (PAGE_SIZE / sizeof(off_t))

// 一页哈希目录块(非空闲链表)  保存每个起始非空闲块页地址
typedef struct {
    off_t directory[HASH_MAP_DIR_BLOCK_SIZE];
} HashMapDirectoryBlock;

// 哈希数据块数目  去掉next和n_items后的数目
#define HASH_MAP_BLOCK_SIZE ((PAGE_SIZE - 2 * sizeof(off_t)) / sizeof(off_t))

// 哈希数据块
typedef struct {
    off_t next;     // 下一个块的地址
    off_t n_items;  // 该块中的空闲空间数目
    off_t table[HASH_MAP_BLOCK_SIZE];
} HashMapBlock;

/* if hash table has already existed,
it will be re-opened and n_directory_blocks will be ignored */
void hash_table_init(const char* filename, BufferPool* pool, off_t n_directory_blocks);

void hash_table_close(BufferPool* pool);

/* there should not be no duplicate addr */
// 标记地址为addr的块有size的空闲空间
void hash_table_insert(BufferPool* pool, short size, off_t addr);

/* if there is no suitable block, return -1 */
// 返回至少包含size空闲空间的块地址
off_t hash_table_pop_lower_bound(BufferPool* pool, short size);

/* addr to be poped must exist */
// 删除地址为addr的块的记录，该块有size的空闲空间
void hash_table_pop(BufferPool* pool, short size, off_t addr);

// 打印哈希块缓冲区
void print_hash_table(BufferPool* pool);

#endif /* _HASH_MAP_H */