#include "block.h"

#include <stdio.h>

// 初始化block
void init_block(Block* block) {
    block->n_items = 0;
    // 相对于block的偏移量
    block->head_ptr = (short)(block->data - (char*)block);
    block->tail_ptr = (short)sizeof(Block);
}

// 返回block第idx项Item的起始地址
ItemPtr get_item(Block* block, short idx) {
    // 超出idx范围
    if (idx < 0 || idx >= block->n_items) {
        printf("get item error: idx is out of range\n");
        return NULL;
    }
    // 获取ItemID
    ItemID item_id = get_item_id(block, idx);
    // 当前Item为空
    if (get_item_id_availability(item_id)) {
        printf("get item error: item_id is not used\n");
        return NULL;
    }
    // 获取Item相对于块的偏移量
    short offset = get_item_id_offset(item_id);
    return (char*)block + offset;
}

// 向block插入大小为item_size，起始地址为item的Item，返回插入后的项号（idx）
short new_item(Block* block, ItemPtr item, short item_size) {}

// 删除block第idx项Item
void delete_item(Block* block, short idx) {}

void str_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    short i;
    printf("\"");
    for (i = 0; i < item_size; i++) {
        printf("%c", item[i]);
    }
    printf("\"");
}

// 打印块的属性及内容
void print_block(Block* block, printer_t printer) {
    printf("----------BLOCK----------\n");
    printf("total = %d\n", block->n_items);  // 已分配的项数
    printf("head = %d\n", block->head_ptr);  // 空闲空间的头指针(偏移量)
    printf("tail = %d\n", block->tail_ptr);  // 空闲空间的尾指针(偏移量)
    for (short i = 0; i < block->n_items; ++i) {
        ItemID item_id = get_item_id(block, i);  // 取得block中第idx项的ItemID
        short availability = get_item_id_availability(item_id);
        short offset = get_item_id_offset(item_id);
        short size = get_item_id_size(item_id);

        ItemPtr item;
        if (!availability)  // 当前Item非空
            item = get_item(block, i);
        else
            item = NULL;

        printf("%10d%5d%10d%10d\t", i, availability, offset, size);
        printer(item, size);
        printf("\n");
    }
    printf("-------------------------\n");
}

// 获取块的状态信息
void analyze_block(Block* block, block_stat_t* stat) {
    stat->empty_item_ids = 0;
    stat->total_item_ids = block->n_items;
    for (short i = 0; i < block->n_items; i++) {
        if (get_item_id_availability(get_item_id(block, i))) {
            ++stat->empty_item_ids;
        }
    }
    // 空闲空间大小(包括空的ItemID)
    stat->available_space =
        block->tail_ptr - block->head_ptr + stat->empty_item_ids * sizeof(ItemID);
}

// 累加块的状态信息 stat += stat2
void accumulate_stat_info(block_stat_t* stat, const block_stat_t* stat2) {
    stat->empty_item_ids += stat2->empty_item_ids;
    stat->total_item_ids += stat2->total_item_ids;
    stat->available_space += stat2->available_space;
}

// 打印块的状态信息
void print_stat_info(const block_stat_t* stat) {
    printf("==========STAT==========\n");
    printf("empty_item_ids: " "%ld" "\n", stat->empty_item_ids);
    printf("total_item_ids: " "%ld" "\n", stat->total_item_ids);
    printf("available_space: " "%ld" "\n", stat->available_space);
    printf("========================\n");
}