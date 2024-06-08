#include "block.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

// 初始化block
void init_block(Block* block) {
    block->n_items = 0;
    block->head_offset = (short)(block->data - (char*)block);
    block->tail_offset = (short)sizeof(Block);
    memset(block->data, 0, sizeof(block->data));
}

// 返回block中第idx项Item的起始地址
ItemPtr get_item(Block* block, short idx) {
    if (idx < 0 || idx >= block->n_items) {
        printf("get_item: idx超出范围\n");
        assert(0);
    }
    ItemID item_id = get_item_id(block, idx);  // 32bit
    if (get_item_id_availability(item_id) == 1) {
        printf("get_item: ItemID指向为空\n");
        assert(0);
    }
    // 获取Item相对于块的偏移量
    short offset = get_item_id_offset(item_id);
    return (char*)block + offset;
}

// 向block插入大小为size的src, 返回插入后的项号(idx)
short new_item(Block* block, char* src, short size) {
    short free_space = block->tail_offset - block->head_offset;
    if (sizeof(ItemID) + size > free_space) {
        printf("new_item: 空间不足\n");
        assert(0);
    }
    block->tail_offset -= size;
    short offset = block->tail_offset;
    ItemID item_id = compose_item_id(0, offset, size);

    // 如果中间有空的ItemID
    // for (int i = 0; i < block->n_items; i++)
    //     if (get_item_id_availability(get_item_id(block, i)) == 1) {
    //         get_item_id(block, i) = item_id;
    //         memcpy((char*)block + offset, src, size);
    //         return i;
    //     }

    // 如果没有可用的ItemID, 则在末尾插入
    memcpy((char*)block + block->head_offset, &item_id, sizeof(ItemID));
    memcpy((char*)block + offset, src, size);
    block->head_offset += sizeof(ItemID);
    block->n_items++;
    return block->n_items - 1;
}

// 删除block第idx项Item
void delete_item(Block* block, short idx) {
    if (idx < 0 || idx >= block->n_items) {
        printf("delete_item: idx超出范围\n");
        assert(0);
    }
    ItemID item_id = get_item_id(block, idx);
    if (get_item_id_availability(item_id)) {
        printf("delete_item: ItemID指向为空\n");
        assert(0);
    }
    short offset = get_item_id_offset(item_id);
    short size = get_item_id_size(item_id);
    get_item_id(block, idx) = compose_item_id(1, 0, 0);

    if (idx < block->n_items - 1) {
        // 将前面的Item向后移动size位
        char temp_buf[PAGE_SIZE];
        memcpy(temp_buf, (char*)block + block->tail_offset, offset - block->tail_offset);
        memcpy((char*)block + block->tail_offset + size, temp_buf,
               offset - block->tail_offset);

        // 更新ItemID的offset, 向后移动size位
        for (int i = idx + 1; i < block->n_items; i++) {
            short temp_aval = get_item_id_availability(get_item_id(block, i));
            short temp_offset = get_item_id_offset(get_item_id(block, i));
            short temp_size = get_item_id_size(get_item_id(block, i));
            get_item_id(block, i) = compose_item_id(temp_aval, temp_offset + size, temp_size);
        }
    }
    block->tail_offset += size;
    return;
}

void str_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    short i;
    printf("\"");
    for (i = 0; i < item_size; i++)
        printf("%c", item[i]);
    printf("\"");
}

// 打印块的属性及内容
void print_block(Block* block, printer_t printer) {
    printf("----------BLOCK----------\n");
    printf("total = %d\n", block->n_items);     // 已分配的项数
    printf("head = %d\n", block->head_offset);  // 空闲空间的头指针(偏移量)
    printf("tail = %d\n", block->tail_offset);  // 空闲空间的尾指针(偏移量)
    for (short idx = 0; idx < block->n_items; idx++) {
        // 取得block中第idx项的ItemID
        ItemID item_id = get_item_id(block, idx);
        short availability = get_item_id_availability(item_id);
        short offset = get_item_id_offset(item_id);
        short size = get_item_id_size(item_id);

        ItemPtr item;
        if (!availability)  // 当前Item非空
            item = get_item(block, idx);
        else
            item = NULL;

        printf("%10d%5d%10d%10d\t", idx, availability, offset, size);
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
        block->tail_offset - block->head_offset + stat->empty_item_ids * sizeof(ItemID);
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
    printf("empty_item_ids: %ld\n", stat->empty_item_ids);
    printf("total_item_ids: %ld\n", stat->total_item_ids);
    printf("available_space: %ld\n", stat->available_space);
    printf("========================\n");
}