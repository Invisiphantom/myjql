#include <string.h>

#include "str.h"

// 根据rid，读取字符串至record
void read_string(Table* table, RID rid, StringRecord* record) {
    table_read(table, rid, record->chunk.data);
    record->idx = 0;
}

// 判断record是否还有下一个字符
int has_next_char(StringRecord* record) {
    // 如果当前chunk未读完
    if (calc_str_chunk_size(record->idx) < get_str_chunk_size(&(record->chunk)))
        return 1;
    // 如果下一个chunk存在
    RID next_rid = get_str_chunk_rid(&(record->chunk));
    if (get_rid_block_addr(next_rid) != -1)
        return 1;
    return 0;
}

// 更新record, 返回下一个字符
char next_char(Table* table, StringRecord* record) {
    // 如果当前chunk未读完
    if (calc_str_chunk_size(record->idx) < get_str_chunk_size(&(record->chunk))) {
        char* chunk_data = get_str_chunk_data_ptr(&(record->chunk));
        char c = chunk_data[record->idx];
        record->idx++;
        return c;
    }
    // 如果下一个chunk存在
    RID next_rid = get_str_chunk_rid(&(record->chunk));
    if (get_rid_block_addr(next_rid) != -1) {
        read_string(table, next_rid, record);
        return next_char(table, record);
    }
}

int compare_string_record(Table* table, const StringRecord* a, const StringRecord* b) {}

// 将长度为size的字符串data写入table表，并返回rid
// 截成每段STR_CHUNK_MAX_LEN, 从后往前插入
RID write_string(Table* table, const char* data, off_t size) {
    RID next_rid;
    get_rid_block_addr(next_rid) = -1;
    while (size > 0) {
        StringChunk chunk;
        get_str_chunk_rid(&chunk) = next_rid;
        off_t start = size - STR_CHUNK_MAX_LEN;
        if (start <= 0) {  // data的短头部size长度
            get_str_chunk_size(&chunk) = calc_str_chunk_size(size);
            memcpy(get_str_chunk_data_ptr(&chunk), data, size);
            next_rid = table_insert(table, chunk.data, get_str_chunk_size(&chunk));
        } else {
            get_str_chunk_size(&chunk) = calc_str_chunk_size(STR_CHUNK_MAX_LEN);
            memcpy(get_str_chunk_data_ptr(&chunk), data + start, STR_CHUNK_MAX_LEN);
            next_rid = table_insert(table, chunk.data, get_str_chunk_size(&chunk));
        }
        size -= STR_CHUNK_MAX_LEN;
    }
    return next_rid;
}

// 根据rid，删除对应字符串
void delete_string(Table* table, RID rid) {
    StringRecord record;
    read_string(table, rid, &record);
    RID next_rid = get_str_chunk_rid(&(record.chunk));
    while (get_rid_block_addr(next_rid) != -1) {
        table_delete(table, rid);
        read_string(table, next_rid, &record);
        rid = next_rid;
        next_rid = get_str_chunk_rid(&(record.chunk));
    }
    return;
}

// 打印record对应的字符串
void print_string(Table* table, const StringRecord* record) {
    StringRecord rec = *record;
    printf("\"");
    while (has_next_char(&rec))
        printf("%c", next_char(table, &rec));
    printf("\"");
}

// 从record加载长度最多为max_size的字符串到dest中，返回加载的字符数 (不包含\0)
size_t load_string(Table* table, const StringRecord* record, char* dest, size_t max_size) {
    int dest_i = 0;
    StringRecord rec = *record;
    while (has_next_char(&rec) && dest_i < max_size) {
        dest[dest_i] = next_char(table, &rec);
        dest_i++;
    }
    return dest_i;
}

// 打印chunk
void chunk_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    StringChunk* chunk = (StringChunk*)item;
    short size = get_str_chunk_size(chunk);
    printf("StringChunk(");
    print_rid(get_str_chunk_rid(chunk));  // 打印rid
    printf(", %d, \"", size);
    for (short i = 0; i < size; i++)
        printf("%c", get_str_chunk_data_ptr(chunk)[i]);  // 打印chunk.data
    printf("\")");
}