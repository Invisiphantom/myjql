#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

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

int test(int test,     // 测试次
         int num_rep,  // 循环次数 1000
         int num_pos,  // 插入备选位置数 256

         int num_ins1,     // 插入数1 512
         int num_pop_lb1,  // 弹出数1 512

         int num_ins2,     // 插入数2 512
         int num_pop,      // 删除数2 256
         int num_pop_lb2,  // 弹出数2 256

         int out_print)  // 是否打印操作输出
{
    int flag = 0;
    BufferPool pool;
    off_t n_directory_blocks = 8;
    hash_table_init("zztest-hashmap", &pool, n_directory_blocks);  // 初始化8个目录块
    int* pos = (int*)malloc(num_pos * sizeof(int));                // pos[num_pos]

    int max_val = n_directory_blocks * HASH_MAP_DIR_BLOCK_SIZE;  // 总共目录项数
    int counter = 1;                                             // 块编号

    m_init(max_val);  // v.resize(max_val);
    printf("max_val = %d\n", max_val);
    for (int rep = 0; rep < num_rep && !flag; rep++) {
        if (rep % 1000 == 0)
            printf("test=%d, rep = %d\n", test, rep);

        //* step 1: select a number of positions to insert a number of addrs
        for (int i = 0; i < num_pos; i++)
            pos[i] = rand() % max_val;

        for (int i = 0; i < num_ins1; i++) {
            int pos_i = rand() % num_pos;  // 随机选取空间大小
            if (out_print)
                printf("insert: 空间大小%d 第%d块\n", pos[pos_i], counter);
            hash_table_insert(&pool, (short)pos[pos_i], counter);
            m_add((short)pos[pos_i], counter);
            counter++;
        }

        //* step 2: pop lower bound
        for (int i = 0; i < num_pop_lb1; i++) {
            int p = rand() % max_val;
            int h_rid = (int)hash_table_pop_lower_bound(&pool, (short)p);
            if (out_print) {
                if (h_rid != -1)  // 如果取到大小合适的块
                    printf("pop lower bound: %d %d (size: %d)\n", p, h_rid, m_get_size(h_rid));
                else
                    printf("pop lower bound: %d %d\n", p, h_rid);
            }

            int p_ans;
            for (p_ans = p; p_ans < max_val; p_ans++)
                if (m_empty((short)p_ans) == 0)
                    break;  // 如果有非空目录项

            if (out_print)
                printf("(answer size: %d)\n", p_ans);

            if (p_ans == max_val) {
                if (h_rid != -1) {
                    printf("* (没有大小合适的块, 需要返回-1) expected: -1\n");
                    flag = 1;
                    break;
                }
            } else {  // 如果存在合适块
                if (h_rid == -1) {
                    printf("* (存在大小合适的块, 不能返回-1) expected size: %d\n", p_ans);
                    flag = 1;
                    break;
                }
                if (m_contain((short)p_ans, h_rid) == 0) {
                    printf("* (返回了错误的rid) expected size: %d\n", p_ans);
                    flag = 1;
                    break;
                }
                m_erase(p_ans, h_rid);
            }
        }
        if (flag == 1)
            break;

        //* step 3: select a number of positions to insert a number of addrs
        for (int i = 0; i < num_pos; i++)
            pos[i] = rand() % max_val;

        for (int i = 0; i < num_ins2; i++) {
            int p = rand() % num_pos;
            hash_table_insert(&pool, (short)pos[p], counter);
            m_add((short)pos[p], counter);
            if (out_print)
                printf("insert: 空间大小%d 第%d块\n", pos[p], counter);
            counter++;
        }

        //* step 4: pop
        for (int i = 0; i < num_pop; i++) {
            if (m_get_total() == 0)
                break;

            // 随机选取rid
            int rid_rand = (int)m_get_item(rand() % (int)m_get_total());
            if (out_print)
                printf("pop: %d (size: %d)\n", rid_rand, m_get_size(rid_rand));
            hash_table_pop(&pool, m_get_size(rid_rand), rid_rand);
            m_erase(m_get_size(rid_rand), rid_rand);
        }

        //* step 5: pop lower bound
        for (int i = 0; i < num_pop_lb2; i++) {
            int p = rand() % max_val;
            int h_rid = (int)hash_table_pop_lower_bound(&pool, (short)p);
            if (out_print) {
                if (h_rid != -1)  // 如果取到大小合适的块
                    printf("pop lower bound: %d %d (size: %d)\n", p, h_rid, m_get_size(h_rid));
                else
                    printf("pop lower bound: %d %d\n", p, h_rid);
            }

            int p_ans;
            for (p_ans = p; p_ans < max_val; p_ans++)
                if (m_empty((short)p_ans) == 0)
                    break;  // 如果有非空目录项

            if (out_print)
                printf("(answer size: %d)\n", p_ans);

            if (p_ans == max_val) {
                if (h_rid != -1) {
                    printf("* (没有大小合适的块, 需要返回-1) expected: -1\n");
                    flag = 1;
                    break;
                }
            } else {  // 如果存在合适块
                if (h_rid == -1) {
                    printf("* (存在大小合适的块, 不能返回-1) expected size: %d\n", p_ans);
                    flag = 1;
                    break;
                }
                if (m_contain((short)p_ans, h_rid) == 0) {
                    printf("* (返回了错误的rid) expected size: %d\n", p_ans);
                    flag = 1;
                    break;
                }
                m_erase(p_ans, h_rid);
            }
        }
    }

    free(pos);
    hash_table_close(&pool);
    if (remove("zztest-hashmap") != 0)
        printf("error deleting: zztest-hashmap\n");
    return flag;
}
int main() {
    if (sizeof(HashMapDirectoryBlock) > PAGE_SIZE) {
        printf("HashMapDirectoryBlock size is too large: %ld\n",
               sizeof(HashMapDirectoryBlock));
        return 1;
    }
    if (sizeof(HashMapBlock) > PAGE_SIZE) {
        printf("HashMapBlock size is too large: %ld\n", sizeof(HashMapBlock));
        return 1;
    }
    
    srand(0);

    printf("BEGIN OF TEST\n");
    char* pt_ptr =
        "循环数=%d, 备选位置数=%d\n"
        "插入数1=%d, 弹出数1=%d\n"
        "插入数2=%d, 删除数2=%d, 弹出数2=%d\n";

    printf("test 1\n");
    printf(pt_ptr, 1, 2, 4, 4, 4, 2, 4);
    if (test(1, 1, 2, 4, 4, 4, 2, 4, 1)) {
        return 1;
    }

    printf("test 2\n");
    printf(pt_ptr, 10000, 10, 40, 40, 40, 20, 20);
    if (test(2, 10000, 10, 40, 40, 40, 20, 20, 0)) {
        return 1;
    }

    // printf("test 3\n");
    // printf(pt_ptr, 1000, 256, 512, 512, 512, 256, 256);
    // if (test(3, 1000, 256, 512, 512, 512, 256, 256, 0)) {
    //     return 1;
    // }

    // printf("test 4\n");
    // printf(pt_ptr, 100000, 1, 1, 1, 1, 1, 0);
    // if (test(4, 100000, 1, 1, 1, 1, 1, 1, 0)) {
    //     return 1;
    // }

    // printf("test 5\n");
    // printf(pt_ptr, 100000, 2, 2, 1, 2, 1, 1);
    // if (test(5, 100000, 2, 2, 1, 2, 1, 1, 0)) {
    //     return 1;
    // }

    printf("END OF TEST\n");
    return 0;
}