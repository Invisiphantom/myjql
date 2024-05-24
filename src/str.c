#include "str.h"

#include "table.h"

void read_string(Table *table, RID rid, StringRecord *record) {
}

int has_next_char(StringRecord *record) {
}

char next_char(Table *table, StringRecord *record) {
}

int compare_string_record(Table *table, const StringRecord *a, const StringRecord *b) {
}

RID write_string(Table *table, const char *data, off_t size) {
}

void delete_string(Table *table, RID rid) {
}

/* void print_string(Table *table, const StringRecord *record) {
    StringRecord rec = *record;
    printf("\"");
    while (has_next_char(&rec)) {
        printf("%c", next_char(table, &rec));
    }
    printf("\"");
} */

size_t load_string(Table *table, const StringRecord *record, char *dest, size_t max_size) {
}

/* void chunk_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    StringChunk *chunk = (StringChunk*)item;
    short size = get_str_chunk_size(chunk), i;
    printf("StringChunk(");
    print_rid(get_str_chunk_rid(chunk));
    printf(", %d, \"", size);
    for (i = 0; i < size; i++) {
        printf("%c", get_str_chunk_data_ptr(chunk)[i]);
    }
    printf("\")");
} */