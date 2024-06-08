#ifndef _STR_H
#define _STR_H

#include "table.h"

#define STR_CHUNK_MAX_SIZE ((PAGE_SIZE) / 4) // chunk最大大小
#define STR_CHUNK_MAX_LEN (STR_CHUNK_MAX_SIZE - sizeof(RID) - sizeof(short)) // chunk.data最大长度

typedef struct {
    /*
     * RID rid: 下一个chunk的rid
     * short size: 当前chunk长度
     * char data[]: 变长字符串
     */
    char data[STR_CHUNK_MAX_SIZE];
} StringChunk;

#define get_str_chunk_rid(chunk) (*(RID*)(chunk)) // chunk.rid
#define get_str_chunk_size(chunk) (*(short*)(((char*)(chunk)) + sizeof(RID))) // chunk.size
#define get_str_chunk_data_ptr(chunk) (((char*)(chunk)) + sizeof(RID) + sizeof(short)) // chunk.data
#define calc_str_chunk_size(len) (sizeof(RID) + sizeof(short) + (len)) // 计算chunk大小

typedef struct {
    StringChunk chunk; // 当前对应的chunk
    short idx;         // 下一个字符的位置
} StringRecord;

// 根据rid，获取对应字符串首位的chunk和idx
void read_string(Table* table, RID rid, StringRecord* record);

// 判断record是否还有下一个字符
int has_next_char(StringRecord* record);

// 更新record, 返回下一个字符
char next_char(Table* table, StringRecord* record);

int compare_string_record(Table* table, const StringRecord* a, const StringRecord* b);

// 将长度为size的字符串data写入table表，并返回rid
RID write_string(Table* table, const char* data, off_t size);

// 根据rid，删除对应字符串
void delete_string(Table* table, RID rid);

// 打印record对应的字符串
void print_string(Table* table, const StringRecord* record);

// 从record加载长度最多为max_size的字符串到dest中，返回加载的字符数(\0需要手动添加)
size_t load_string(Table* table, const StringRecord* record, char* dest, size_t max_size);

// 打印chunk
void chunk_printer(ItemPtr item, short item_size);

#endif /* _STR_H */