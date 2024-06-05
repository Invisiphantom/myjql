#ifndef _BUFFER_POOL_H
#define _BUFFER_POOL_H

#include "file_io.h"
// 文件读写调用file_io.h中的读写函数

#define CACHE_PAGE 8

typedef struct {
    /* you can modify anything in this struct */
    FileInfo file;
    Page pages[CACHE_PAGE];
    off_t addrs[CACHE_PAGE];
    size_t cnt[CACHE_PAGE];
    size_t ref[CACHE_PAGE];
} BufferPool;

// 打开filename，并关联pool为其缓冲池
void init_buffer_pool(const char* filename, BufferPool* pool);

// 关闭缓冲池，将缓冲的页写回文件
void close_buffer_pool(BufferPool* pool);

// 获取地址为addr的页，并锁定（保证该页不会被意外换出）
Page* get_page(BufferPool* pool, off_t addr);

// 释放地址为addr的页，该页之后可以被换出
void release(BufferPool* pool, off_t addr);

/* void print_buffer_pool(BufferPool *pool); */

/* void validate_buffer_pool(BufferPool *pool); */

#endif /* _BUFFER_POOL_H */