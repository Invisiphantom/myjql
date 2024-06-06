#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "buffer_pool.h"
#include "hash_map.h"

typedef long my_off_t;

void m_init(size_t size);
void m_add(short size, my_off_t id);
void m_erase(short size, my_off_t id);
int m_contain(short size, my_off_t id);
int m_empty(short size);
short m_get_size(my_off_t id);
size_t m_get_total();
my_off_t m_get_item(int idx);

int test(int num_rep,
         int num_pos,
         int num_ins1,
         int num_pop_lb1,
         int num_ins2,
         int num_pop,
         int num_pop_lb2,
         int out)  // 是否打印操作输出
{
    int flag = 0;
    BufferPool pool;
    hash_table_init("zztest-hashmap", &pool, 8);
    int* pos = (int*)malloc(num_pos * sizeof(int));  // pos[num_pos]

    int max_val = 8 * HASH_MAP_DIR_BLOCK_SIZE;
    int counter = 1;
    int r, a;

    m_init(max_val);
    printf("max_val = %d\n", max_val);
    for (int rep = 0; rep < num_rep && !flag; rep++) {
        //* step 1: select a number of positions to insert a number of addrs
        for (int i = 0; i < num_pos; ++i) {
            pos[i] = rand() % max_val;
        }
        for (int i = 0; i < num_ins1; ++i) {
            int p = rand() % num_pos;
            hash_table_insert(&pool, (short)pos[p], counter);
            m_add((short)pos[p], counter);
            if (out)
                printf("insert: %d %d\n", pos[p], counter);
            counter++;
        }
        //* step 2: pop lower bound
        for (int i = 0; i < num_pop_lb1; ++i) {
            int p = rand() % max_val;
            r = (int)hash_table_pop_lower_bound(&pool, (short)p);
            if (out) {
                if (r != -1) {
                    printf("pop lower bound: %d %d (size: %d)\n", p, r, m_get_size(r));
                } else {
                    printf("pop lower bound: %d %d\n", p, r);
                }
            }
            a = p;
            while (a < max_val) {
                if (!m_empty((short)a)) {
                    break;
                }
                ++a;
            }
            if (out)
                printf("answer size: %d\n", a);
            if (a == max_val) { /* a == -1 */
                if (r != -1) {
                    printf("* expected: -1\n");
                    flag = 1;
                    break;
                } /* else r == -1, OK */
            } else { /* a != -1 */
                if (r == -1) {
                    printf("* expected size: %d\n", a);
                    flag = 1;
                    break;
                }
                /* r != -1 */
                if (!m_contain((short)a, r)) {
                    printf("* expected size: %d\n", a);
                    flag = 1;
                    break;
                }
                /* contain((short)a, r), OK */
                m_erase(a, r);
            }
        }
        if (flag) {
            break;
        }
        //* step 3: select a number of positions to insert a number of addrs
        for (int i = 0; i < num_pos; ++i) {
            pos[i] = rand() % max_val;
        }
        for (int i = 0; i < num_ins2; ++i) {
            int p = rand() % num_pos;
            hash_table_insert(&pool, (short)pos[p], counter);
            m_add((short)pos[p], counter);
            if (out)
                printf("insert: %d %d\n", pos[p], counter);
            counter++;
        }
        //* step 4: pop
        for (int i = 0; i < num_pop; ++i) {
            if (m_get_total() == 0) {
                break;
            }
            r = (int)m_get_item(rand() % (int)m_get_total());
            if (out)
                printf("pop: %d (size: %d)\n", r, m_get_size(r));
            hash_table_pop(&pool, m_get_size(r), r);
            m_erase(m_get_size(r), r);
        }
        //* step 5: pop lower bound
        for (int i = 0; i < num_pop_lb2; ++i) {
            int p = rand() % max_val;
            r = (int)hash_table_pop_lower_bound(&pool, (short)p);
            if (out) {
                if (r != -1) {
                    printf("pop lower bound: %d %d (size: %d)\n", p, r, m_get_size(r));
                } else {
                    printf("pop lower bound: %d %d\n", p, r);
                }
            }
            a = p;
            while (a < max_val) {
                if (!m_empty((short)a)) {
                    break;
                }
                ++a;
            }
            if (out)
                printf("answer size: %d\n", a);
            if (a == max_val) { /* a == -1 */
                if (r != -1) {
                    printf("* expected: -1\n");
                    flag = 1;
                    break;
                } /* else r == -1, OK */
            } else { /* a != -1 */
                if (r == -1) {
                    printf("* expected size: %d\n", a);
                    flag = 1;
                    break;
                }
                /* r != -1 */
                if (!m_contain((short)a, r)) {
                    printf("* expected size: %d\n", a);
                    flag = 1;
                    break;
                }
                /* contain((short)a, r), OK */
                m_erase(a, r);
            }
        }
    }

    free(pos);

    /* validation & cleanup */
    /* validate_buffer_pool(&pool); */
    hash_table_close(&pool);
    if (remove("zztest-hashmap") != 0) {
        printf("error deleting: zztest-hashmap\n");
    }
    return flag;
}
int main() {
    if (sizeof(HashMapDirectoryBlock) > PAGE_SIZE) {
        printf("HashMapDirectoryBlock size is too large: " FORMAT_SIZE_T "\n",
               sizeof(HashMapDirectoryBlock));
        return 1;
    }
    if (sizeof(HashMapBlock) > PAGE_SIZE) {
        printf("HashMapBlock size is too large: " FORMAT_SIZE_T "\n", sizeof(HashMapBlock));
        return 1;
    }

    /* fixed random seed */
    srand(0);
    // srand((unsigned int)time(NULL));

    // if (test(1, 2, 4, 4, 4, 2, 4, 1)) {
    //     return 1;
    // }
    if (test(10000, 10, 40, 40, 40, 20, 20, 0)) {
        return 1;
    }
    if (test(1000, 256, 512, 512, 512, 256, 256, 0)) {
        return 1;
    }
    if (test(100000, 1, 1, 1, 1, 1, 1, 0)) {
        return 1;
    }
    if (test(100000, 2, 2, 1, 2, 1, 1, 0)) {
        return 1;
    }

    /* prevent using exit(0) to pass the test */
    printf("END OF TEST\n");
    return 0;
}