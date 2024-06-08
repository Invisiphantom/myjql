#ifndef _BLOCK_H
#define _BLOCK_H

#include "file_io.h"

// 页块
typedef struct {
    short n_items;                             // 已分配的项数
    short head_offset;                         // 空闲空间的头偏移量
    short tail_offset;                         // 空闲空间的尾偏移量
    char data[PAGE_SIZE - 3 * sizeof(short)];  //  空闲空间(对齐PAGE_SIZE)
} Block;

/*
 * ItemID: 32-bit unsigned int
 * 31-th bit: not used, always be 0
 * 30-th bit: availability
 *     0: unavailable (because it has been used)
 *     1: available   (for future use)
 * 29~15 bit: offset 偏移位置(15bits)
 * 14~0  bit: size   项大小  (15bits)
 */

typedef unsigned int ItemID;  // 32bit
typedef char* ItemPtr;        // 8bit

#define get_item_id_availability(item_id) (((item_id) >> 30) & 1)
#define get_item_id_offset(item_id) (((item_id) >> 15) & ((1 << 15) - 1))
#define get_item_id_size(item_id) ((item_id) & ((1 << 15) - 1))

// 构建ItemID
#define compose_item_id(availability, offset, size)                        \
    ((((availability) & 1) << 30) | (((offset) & ((1 << 15) - 1)) << 15) | \
     ((size) & ((1 << 15) - 1)))

// 取得block中第idx项的ItemID
#define get_item_id(block, idx) (*(ItemID*)((block)->data + sizeof(ItemID) * (idx)))

// 初始化block
void init_block(Block* block);

// 返回block中第idx项Item的起始地址
ItemPtr get_item(Block* block, short idx);

// 向block插入大小为size的src, 返回插入后的项号(idx)
short new_item(Block* block, char* src, short size);

// 删除block第idx项Item
void delete_item(Block* block, short idx);

typedef void (*printer_t)(ItemPtr item, short item_size);

// 用字符串形式打印Item
void str_printer(ItemPtr item, short item_size);

// 打印块的属性及内容
void print_block(Block* block, printer_t printer);

// 块的状态信息
typedef struct {
    size_t empty_item_ids;   // 空闲Item数
    size_t total_item_ids;   // 总的Item数
    size_t available_space;  // 空闲空间大小
} block_stat_t;

// 获取块的状态信息
void analyze_block(Block* block, block_stat_t* stat);

// 累加块的状态信息 stat += stat2
void accumulate_stat_info(block_stat_t* stat, const block_stat_t* stat2);

// 打印块的状态信息
void print_stat_info(const block_stat_t* stat);

#endif /* _BLOCK_H */