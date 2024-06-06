#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "table.h"
#include "str.h"

typedef long my_off_t;

typedef struct {
    my_off_t addr;
    short idx;
} my_RID;

void m_init();
void m_insert(my_RID rid, char* s);
void m_erase(my_RID rid);
size_t m_get_total();
my_RID m_get_rid(int idx);
int m_equal(my_RID rid, char* s);

#define N 1024

char buf[N + 1];

char random_char() {
    int op = rand() % 3;
    if (op == 0) {
        return 'a' + rand() % 26;
    } else if (op == 1) {
        return 'A' + rand() % 26;
    } else {
        return '0' + rand() % 10;
    }
}

int generate_string(int n) {
    int len = rand() % n;
    int i;
    for (i = 0; i < len; ++i) {
        buf[i] = random_char();
    }
    buf[len] = 0;
    return len;
}

// 最长字符长度, 操作数, 是否打印操作输出
// max_str_len=512, num_op=10000, out=0
int test(int max_str_len, int num_op, int out) {
    int flag = 0;
    RID rid;
    my_RID m_rid;
    StringRecord rec;

    Table table;
    table_init(&table, "zztest-str.data", "zztest-str.fsm");

    for (int i = 0; i < num_op; ++i) {
        int op = rand() % 3;
        int m_size = m_get_total();
        //* read
        if (op == 0 && m_size != 0) {
            // 随机选取 m_rid
            m_rid = m_get_rid(rand() % m_size);
            get_rid_block_addr(rid) = m_rid.addr;
            get_rid_idx(rid) = m_rid.idx;
            if (out) {
                printf("read: ");
                print_rid(rid);
                printf("\n");
            }
            // 根据rid，获取对应字符串的chunk和idx
            read_string(&table, rid, &rec);
            // 从rec加载长度最多为N的字符串到buf中
            int n = (int)load_string(&table, &rec, buf, N);
            buf[n] = 0;
            // 比较buf和m_rid对应的字符串是否相等
            if (m_equal(m_rid, buf)) {
                if (out)
                    printf("OK\n");
            } else {
                printf("* error\n");
                flag = 1;
                break;
            }
        }
        //* delete
        if (op == 1 && m_size != 0) {
            m_rid = m_get_rid(rand() % m_size);
            get_rid_block_addr(rid) = m_rid.addr;
            get_rid_idx(rid) = m_rid.idx;
            if (out) {
                printf("delete: ");
                print_rid(rid);
                printf("\n");
            }
            delete_string(&table, rid);
            m_erase(m_rid);
        }
        //* write
        else {
            int n = generate_string(max_str_len);
            rid = write_string(&table, buf, n);
            m_rid.addr = get_rid_block_addr(rid);
            m_rid.idx = get_rid_idx(rid);
            m_insert(m_rid, buf);
            if (out) {
                printf("insert: %s, len = %d @ ", buf, n);
                print_rid(rid);
                printf("\n");
            }
        }
    }

    /* validate_buffer_pool(&table.data_pool); */
    /* validate_buffer_pool(&table.fsm_pool); */
    table_close(&table);

    if (remove("zztest-str.data")) {
        printf("error deleting: zztest-str.data\n");
    }
    if (remove("zztest-str.fsm")) {
        printf("error deleting: zztest-str.fsm\n");
    }

    return flag;
}

int main() {
    srand(0);

    // if (test(512, 10000, 0)) {
    if (test(10, 30, 1)) {
        return 1;
    }

    printf("END OF TEST\n");
    return 0;
}