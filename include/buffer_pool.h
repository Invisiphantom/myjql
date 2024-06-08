#ifndef _BUFFER_POOL_H
#define _BUFFER_POOL_H

#include "file_io.h"

#define CACHE_PAGE 16
typedef struct {
    FileInfo file; // 文件信息
    Page pages[CACHE_PAGE]; // 页数组
    off_t addrs[CACHE_PAGE]; // 地址数组
    size_t age[CACHE_PAGE]; // 计数数组(LRU)
    size_t ref[CACHE_PAGE]; // 引用数组(锁定)
} BufferPool;

// 打开filename，并关联pool为其缓冲池
FileIOResult init_buffer_pool(const char* filename, BufferPool* pool);

// 关闭缓冲池，将缓冲的页写回文件
void close_buffer_pool(BufferPool* pool);

// 获取地址为addr的页，并锁定(保证该页不会被意外换出)
Page* get_page(BufferPool* pool, off_t addr);

// 释放地址为addr的页，该页之后可以被换出
void release(BufferPool* pool, off_t addr);

/* void print_buffer_pool(BufferPool *pool); */

/* void validate_buffer_pool(BufferPool *pool); */

#endif /* _BUFFER_POOL_H */