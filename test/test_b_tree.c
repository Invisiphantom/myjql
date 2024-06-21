#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "b_tree.h"

typedef long my_off_t;

void m_init();
void m_insert(my_off_t addr, short idx);
void m_erase(my_off_t addr);
short m_get_idx(my_off_t addr);
int m_contain(my_off_t addr);
size_t m_get_total();
my_off_t m_get_addr(int idx);

RID get_rand_rid() {
    RID rid;
    get_rid_block_addr(rid) = rand();
    get_rid_idx(rid) = (short)rand();
    return rid;
}

int op(off_t a, off_t b) {
    if (a > b)
        return 1;
    else if (a < b)
        return -1;
    else
        return 0;
}

int rid_row_row_cmp(RID a, RID b) {
    return op(get_rid_block_addr(a), get_rid_block_addr(b));
}

int rid_ptr_row_cmp(void* p, size_t size, RID b) {
    off_t a = *(off_t*)p;
    return op(a, get_rid_block_addr(b));
}

RID insert_handler(RID rid) {
    RID new_rid;
    get_rid_block_addr(new_rid) = get_rid_block_addr(rid);
    get_rid_idx(new_rid) = -1;
    return new_rid;
}

void delete_handler(RID rid) {}

int test(int num_op, int out) {
    int flag = 0;
    BufferPool pool;
    b_tree_init("zztest-b-tree", &pool);
    m_init();

    for (int i = 0; i < num_op; i++) {
        int op = rand() % 3;
        RID rid, rid_get, rid_ans;
        if(out)
            printf("op: %d\n", i);

        // 插入操作
        if (op == 0) {
            do {
                rid = get_rand_rid();  // 随机块及块内索引
            } while (m_contain(get_rid_block_addr(rid)));
            // 每个block只存一个idx

            if (out) {
                printf("insert: ");
                print_rid(rid);
                printf("\n");
            }

            b_tree_insert(&pool, rid, &rid_row_row_cmp, &insert_handler);
            m_insert(get_rid_block_addr(rid), get_rid_idx(rid));
        }

        // 删除操作
        else if (op == 1 && m_get_total() != 0) {
            get_rid_block_addr(rid) = m_get_addr(rand() % (int)m_get_total());
            get_rid_idx(rid) = 0;  // 每个block只存一个idx

            if (out) {
                printf("erase: ");
                print_rid(rid);
                printf("\n");
            }

            // 删除rid.block_addr对应的结点项
            b_tree_delete(&pool, rid, &rid_row_row_cmp, &insert_handler, &delete_handler);
            m_erase(get_rid_block_addr(rid));
        }

        // 查找操作
        else {
            if (rand() % 2 && m_get_total() != 0)  // 查找已有的键(block_addr)
                get_rid_block_addr(rid) = m_get_addr(rand() % (int)m_get_total());
            else  // 查找随机块(可能不存在)
                rid = get_rand_rid();

            get_rid_idx(rid) = 0;

            if (out) {
                printf("search: ");
                print_rid(rid);
                printf("\n");
            }

            // 查找rid.block_addr对应的结点项
            rid_get = b_tree_search(&pool, &rid, sizeof(rid), &rid_ptr_row_cmp);

            if (m_contain(get_rid_block_addr(rid))) {
                get_rid_block_addr(rid_ans) = get_rid_block_addr(rid);
                get_rid_idx(rid_ans) = m_get_idx(get_rid_block_addr(rid));
            } else {
                get_rid_block_addr(rid_ans) = -1;
                get_rid_idx(rid_ans) = 0;
            }

            if (out) {
                printf("expected: ");
                print_rid(rid_ans);
                printf(", got: ");
                print_rid(rid_get);
                printf("\n");
            }

            if (get_rid_block_addr(rid_get) == get_rid_block_addr(rid_ans) &&
                get_rid_idx(rid_get) == get_rid_idx(rid_ans)) {
                if (out)
                    printf("OK\n");
            } else {
                printf("* error: \n");
                printf("expected: ");
                print_rid(rid_ans);
                printf(", got: ");
                print_rid(rid_get);
                printf("\n");
                flag = 1;
                break;
            }
        }

        // if (out)
        //     b_tree_print(&pool);
    }

    /* b_tree_traverse(&pool); */
    /* validate_buffer_pool(&pool); */
    b_tree_close(&pool);

    if (remove("zztest-b-tree"))
        printf("error deleting: zztest-b-tree\n");
    return flag;
}

int main() {
    printf("PAGE_SIZE = %d\n", PAGE_SIZE);
    printf("DEGREE = %ld\n", DEGREE);
    printf("BNode size: %ld\n", sizeof(BNode));

    if (DEGREE < 2) {
        printf("error: DEGREE < 2\n");
        return 1;
    }
    if (sizeof(BNode) > PAGE_SIZE) {
        printf("error: BNode size is too large\n");
        return 1;
    }

    srand(0);

    // if (test(50, 1)) {
    //     return 1;
    // }

    if (test(2000000, 1)) {
        return 1;
    }

    printf("END OF TEST\n");
    return 0;
}