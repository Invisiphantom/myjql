#ifndef _HASH_MAP_H
#define _HASH_MAP_H

#include "buffer_pool.h"

/* static hash map for free space map */

typedef struct {
    /* you can modify anything in this struct */
    off_t free_block_head;
    off_t n_directory_blocks;
    off_t max_size;
} HashMapControlBlock;

#define HASH_MAP_DIR_BLOCK_SIZE (PAGE_SIZE / sizeof(off_t))

typedef struct {
    /* you can modify anything in this struct */
    off_t directory[HASH_MAP_DIR_BLOCK_SIZE];
} HashMapDirectoryBlock;

#define HASH_MAP_BLOCK_SIZE ((PAGE_SIZE - 2 * sizeof(off_t)) / sizeof(off_t))

typedef struct {
    /* you can modify anything in this struct */
    off_t next;
    off_t n_items;
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

/* void print_hash_table(BufferPool *pool); */

#endif /* _HASH_MAP_H */