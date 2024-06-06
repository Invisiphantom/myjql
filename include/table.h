#ifndef _TABLE_H
#define _TABLE_H

#include "buffer_pool.h"
#include "hash_map.h"
#include "block.h"

// 全局表
typedef struct {
    BufferPool data_pool;  // 数据缓冲区
    BufferPool fsm_pool;   // 哈希块缓冲区
} Table;

typedef struct {
    // off_t block_addr,
    // short idx
    char data[sizeof(off_t) + sizeof(short)];  // 强制对齐10字节
} RID;

#define get_rid_block_addr(rid) (*(off_t*)(&(rid)))                      // RID.block_addr
#define get_rid_idx(rid) (*(short*)(((char*)(&(rid))) + sizeof(off_t)))  // RID.idx

// 初始化全局表
void table_init(Table* table, const char* data_filename, const char* fsm_filename);

// 关闭全局表
void table_close(Table* table);

// 获取数据缓冲区的页数
off_t table_get_total_blocks(Table* table);

// 获取页块的item数
short table_block_get_total_items(Table* table, off_t block_addr);

// 根据rid，将数据读入dest，需要确保dest拥有适当的大小
void table_read(Table* table, RID rid, ItemPtr dest);

// 插入大小为size，起始地址为src的Item，返回rid
RID table_insert(Table* table, ItemPtr src, short size);

// 根据rid，删除相应Item
void table_delete(Table* table, RID rid);

// 打印全局表
void print_table(Table* table, printer_t printer);

// 打印 RID.block_addr 和 RID.idx
void print_rid(RID rid);

// 获取全局表的统计信息
void analyze_table(Table* table);

#endif /* _TABLE_H */