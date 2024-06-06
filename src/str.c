#include "str.h"

#include "table.h"

// 根据rid，获取对应字符串的chunk和idx
void read_string(Table* table, RID rid, StringRecord* record) {}

// 判断record是否还有下一个字符
int has_next_char(StringRecord* record) {}

// 获取record的下一个字符
char next_char(Table* table, StringRecord* record) {}

int compare_string_record(Table* table, const StringRecord* a, const StringRecord* b) {}

// 将长度为size的字符串data写入table表，并返回rid
RID write_string(Table* table, const char* data, off_t size) {}

// 根据rid，删除对应字符串
void delete_string(Table* table, RID rid) {}

// 打印record对应的字符串
void print_string(Table *table, const StringRecord *record) {
    StringRecord rec = *record;
    printf("\"");
    while (has_next_char(&rec)) {
        printf("%c", next_char(table, &rec));
    }
    printf("\"");
} 

// 从record加载长度最多为max_size的字符串到dest中，返回加载的字符数(\0需要手动添加)
size_t load_string(Table* table, const StringRecord* record, char* dest, size_t max_size) {}

// 打印chunk
void chunk_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    StringChunk *chunk = (StringChunk*)item;
    short size = get_str_chunk_size(chunk);
    printf("StringChunk(");
    print_rid(get_str_chunk_rid(chunk)); // 打印rid
    printf(", %d, \"", size);
    for (short i = 0; i < size; i++) {
        printf("%c", get_str_chunk_data_ptr(chunk)[i]); // 打印chunk.data
    }
    printf("\")");
} 