#include <stdio.h>
#include <string.h>

#include "table.h"

// 初始化全局表
void table_init(Table* table, const char* data_filename, const char* fsm_filename) {
    init_buffer_pool(data_filename, &table->data_pool);
    hash_table_init(fsm_filename, &table->fsm_pool, PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE);
}

// 关闭全局表
void table_close(Table* table) {
    close_buffer_pool(&table->data_pool);
    hash_table_close(&table->fsm_pool);
}

// 获取数据缓冲区的页数
off_t table_get_total_blocks(Table* table) {
    return table->data_pool.file.length / PAGE_SIZE;
}

// 获取页块的item数
short table_block_get_total_items(Table* table, off_t block_addr) {
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    short n_items = block->n_items;
    release(&table->data_pool, block_addr);
    return n_items;
}

// 标记地址为addr的块有size的空闲空间
// void hash_table_insert(BufferPool* pool, short size, off_t addr);

// 新增num_blocks个空块
void table_new_blocks(Table* table, int num_blocks) {
    off_t file_length = table->data_pool.file.length;
    off_t tail_i = file_length / PAGE_SIZE;
    for (off_t i = 0; i < num_blocks; i++) {
        Block* block = (Block*)get_page(&table->data_pool, (tail_i + i) * PAGE_SIZE);
        init_block(block);
        release(&table->data_pool, (tail_i + i) * PAGE_SIZE);
        hash_table_insert(&table->fsm_pool, block->tail_offset - block->head_offset,
                          (tail_i + i) * PAGE_SIZE);
    }
    return;
}

// 根据rid，将数据读入dest，需要确保dest拥有适当的大小
void table_read(Table* table, RID rid, char* dest) {
    off_t block_addr = get_rid_block_addr(rid);
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    short idx = get_rid_idx(rid);
    short size = get_item_id_size(get_item_id(block, idx));
    ItemPtr item = get_item(block, idx);
    memcpy(dest, item, size);
    release(&table->data_pool, block_addr);
}

// 插入大小为size，起始地址为src的Item，返回rid
RID table_insert(Table* table, char* src, short size) {
    off_t block_addr = hash_table_pop_lower_bound(&table->fsm_pool, size + sizeof(ItemID));
    if (block_addr == -1) {
        table_new_blocks(table, 8);
        block_addr = hash_table_pop_lower_bound(&table->fsm_pool, size + sizeof(ItemID));
    }
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    short idx = new_item(block, src, size);
    release(&table->data_pool, block_addr);
    hash_table_insert(&table->fsm_pool, block->tail_offset - block->head_offset, block_addr);
    
    RID rid;
    get_rid_block_addr(rid) = block_addr;
    get_rid_idx(rid) = idx;
    return rid;
}

// 根据rid，删除相应Item
void table_delete(Table* table, RID rid) {
    off_t block_addr = get_rid_block_addr(rid);
    Block* block = (Block*)get_page(&table->data_pool, block_addr);
    release(&table->data_pool, block_addr);

    hash_table_pop(&table->fsm_pool, block->tail_offset - block->head_offset, block_addr);
    delete_item(block, get_rid_idx(rid));
    hash_table_insert(&table->fsm_pool, block->tail_offset - block->head_offset, block_addr);
}

// 打印全局表
void print_table(Table* table, printer_t printer) {
    printf("\n---------------TABLE---------------\n");
    off_t total = table_get_total_blocks(table);  // 数据页块数
    for (off_t i = 0; i < total; i++) {
        off_t block_addr = i * PAGE_SIZE;  // 第i个页块的地址
        Block* block = (Block*)get_page(&table->data_pool, block_addr);
        printf("[%ld]\n", block_addr);
        print_block(block, printer);  // 打印每个页块
        release(&table->data_pool, block_addr);
    }
    printf("***********************************\n");
    print_hash_table(&table->fsm_pool);  // 打印哈希块缓冲区
    printf("-----------------------------------\n\n");
}

// 打印 RID.block_addr 和 RID.idx
void print_rid(RID rid) {
    printf("RID(%ld, %d)", get_rid_block_addr(rid), get_rid_idx(rid));
}

// 获取全局表的统计信息
void analyze_table(Table* table) {
    block_stat_t stat_sum, curr;
    stat_sum.empty_item_ids = 0;
    stat_sum.total_item_ids = 0;
    stat_sum.available_space = 0;
    off_t total = table_get_total_blocks(table);  // 数据页块数
    off_t total_size = total * PAGE_SIZE;         // 数据页总大小
    for (off_t i = 0; i < total; i++) {
        off_t block_addr = i * PAGE_SIZE;
        Block* block = (Block*)get_page(&table->data_pool, block_addr);  //* 锁定
        analyze_block(block, &curr);
        release(&table->data_pool, block_addr);  //* 释放
        accumulate_stat_info(&stat_sum, &curr);  // 累加每个块的状态信息
    }
    printf("----------ANALYSIS----------\n");
    printf("total blocks: %ld\n", total);
    printf("total size: %ld\n", total_size);
    printf("total occupancy: %.4f\n", 1.0f - 1.0f * stat_sum.available_space / total_size);
    printf("ItemID非空率: %.4f\n",
           1.0f - 1.0f * stat_sum.empty_item_ids / stat_sum.total_item_ids);
    printf("----------------------------\n\n");
}