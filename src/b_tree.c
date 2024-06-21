#include "b_tree.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MAX_NODE_NUM 200000

// 如果B树已经存在，则重新打开, 否则初始化
void b_tree_init(const char* filename, BufferPool* pool) {
    FileIOResult res = init_buffer_pool(filename, pool);
    if (res == FILE_OPEN) {  // 如果哈希表已经存在，则重新打开
        printf("B Tree already exists, reopen\n");
        return;
    }

    // 初始化控制块
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    ctrl->root_node = 1 * PAGE_SIZE;
    ctrl->free_node_head = 2 * PAGE_SIZE;
    release(pool, 0);  //* 释放控制块

    // 初始化根节点块
    BNode* root_node = (BNode*)get_page(pool, 1 * PAGE_SIZE);  //* 锁定根结点块
    root_node->n = 0;
    root_node->leaf = 1;
    root_node->lBro = -1;
    root_node->rBro = -1;
    release(pool, 1 * PAGE_SIZE);  //* 释放根结点块

    // 初始化空闲块链表
    for (off_t fb_i = 2; fb_i < MAX_NODE_NUM; fb_i++) {
        BNode* node = (BNode*)get_page(pool, fb_i * PAGE_SIZE);  //* 锁定结点块
        node->next = (fb_i + 1) * PAGE_SIZE;
        release(pool, fb_i * PAGE_SIZE);  //* 释放结点块
    }
    // 初始化空闲块链表的末尾块
    BNode* node = (BNode*)get_page(pool, MAX_NODE_NUM * PAGE_SIZE);  //* 锁定结点块
    node->next = -1;
    release(pool, MAX_NODE_NUM * PAGE_SIZE);  //* 释放结点块
}

void b_tree_close(BufferPool* pool) {
    close_buffer_pool(pool);
}

// ------------------------------------------------

// 从空闲块链表中分配一个块
off_t b_tree_alloc(BufferPool* pool) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    if (ctrl->free_node_head == -1) {
        fprintf(stderr, "No free node\n");
        assert(0);
    }
    off_t node_addr = ctrl->free_node_head;
    BNode* node = (BNode*)get_page(pool, node_addr);  //* 锁定结点块
    ctrl->free_node_head = node->next;
    release(pool, node_addr);  //* 释放结点块
    release(pool, 0);          //* 释放控制块
    assert((node_addr & PAGE_MASK) == 0);
    return node_addr;
}

// 释放地址为addr的块到空闲块链表
void b_tree_free(BufferPool* pool, off_t addr) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    BNode* node = (BNode*)get_page(pool, addr);         //* 锁定结点块
    node->next = ctrl->free_node_head;
    ctrl->free_node_head = addr;
    release(pool, addr);  //* 释放结点块
    release(pool, 0);     //* 释放控制块
}

// ------------------------------------------------

off_t b_tree_search_helper(BufferPool* pool,
                           off_t nodeAddr,
                           void* key,
                           size_t size,
                           b_tree_ptr_row_cmp_t cmp) {
    BNode* node = (BNode*)get_page(pool, nodeAddr);  //* 锁定结点块
    if (node->leaf) {
        for (size_t i = 0; i < node->n; i++) {
            // 如果找到匹配项
            if (cmp(key, size, node->row_ptr[i]) == 0) {
                release(pool, nodeAddr);  //* 释放结点块
                return nodeAddr;          // 返回结点地址
            }
        }
        // 没有找到匹配项
        release(pool, nodeAddr);  //* 释放结点块
        return -1;                // 返回-1
    }

    for (size_t i = 0; i < node->n; i++) {
        if (cmp(key, size, node->row_ptr[i]) < 0) {
            release(pool, nodeAddr);  //* 释放结点块
            off_t res = b_tree_search_helper(pool, node->child[i], key, size, cmp);
            return res;
        }
    }

    release(pool, nodeAddr);  //* 释放结点块
    off_t res = b_tree_search_helper(pool, node->child[node->n], key, size, cmp);
    return res;
}

// 查找key=rid.block_addr对应的结点项
RID b_tree_search(BufferPool* pool, void* key, size_t size, b_tree_ptr_row_cmp_t cmp) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    release(pool, 0);                                   //* 释放控制块

    off_t res = b_tree_search_helper(pool, ctrl->root_node, key, size, cmp);

    RID rid;
    if (res == -1) {
        get_rid_block_addr(rid) = -1;
        get_rid_idx(rid) = 0;
    } else {
        BNode* node = (BNode*)get_page(pool, res);  //* 锁定结点块
        for (size_t i = 0; i < node->n; i++) {
            if (cmp(key, size, node->row_ptr[i]) == 0) {
                rid = node->row_ptr[i];
                break;
            }
        }
        release(pool, res);  //* 释放结点块
    }
    return rid;
}

void b_tree_insert_helper(BufferPool* pool,          // 缓冲池
                          off_t nodeAddr,            // 当前结点地址
                          RID rid,                   // 需要插入的rid
                          RID* newChildRid_ptr,      // 上插的最小rid
                          off_t* newChildAddr_ptr,   // 上插的结点地址
                          b_tree_row_row_cmp_t cmp,  // rid比较函数
                          b_tree_insert_nonleaf_handler_t insert_handler) {
    BNode* node = (BNode*)get_page(pool, nodeAddr);  //* 锁定结点块

    // 如果结点是叶子结点, 记为L
    if (node->leaf) {
        // 如果L有空间(通常情况), 直接插入
        if (node->n < 2 * DEGREE - 1) {
            // 找到插入位置
            size_t i = node->n;
            while (i > 0 && cmp(rid, node->row_ptr[i - 1]) < 0) {
                node->row_ptr[i] = node->row_ptr[i - 1];
                i--;
            }
            node->row_ptr[i] = rid;
            node->n++;
            release(pool, nodeAddr);  //* 释放结点块
            return;                   //! 返回
        }

        // 如果L已满, 则分裂L
        // newChildRid=L2.rid[0], newChildAddr=L2.addr
        else {
            off_t newAddr = b_tree_alloc(pool);
            BNode* newNode = (BNode*)get_page(pool, newAddr);  //* 锁定新结点块
            newNode->leaf = 1;                                 // 新结点是叶子结点
            newNode->lBro = -1;
            newNode->rBro = -1;
            // 如果rid<L.rid[DEGREE-1], 则将L的后DEGREE项移至L2, 并向L插入rid
            if (cmp(rid, node->row_ptr[DEGREE - 1]) < 0) {
                for (size_t i = 0; i < DEGREE; i++)
                    newNode->row_ptr[i] = node->row_ptr[i + DEGREE - 1];  // {0,1,2}={2,3,4}
                node->n = DEGREE - 1;
                newNode->n = DEGREE;

                size_t i = node->n;
                while (i > 0 && cmp(rid, node->row_ptr[i - 1]) < 0) {
                    node->row_ptr[i] = node->row_ptr[i - 1];
                    i--;
                }
                node->row_ptr[i] = rid;
                node->n++;
            }
            // 如果rid>=L.rid[DEGREE-1], 则将L的后DEGREE-1项移至L2, 并向L2插入rid
            else {
                for (size_t i = 0; i < DEGREE - 1; i++)
                    newNode->row_ptr[i] = node->row_ptr[i + DEGREE];  // {0,1}={3,4}
                node->n = DEGREE;
                newNode->n = DEGREE - 1;

                size_t i = newNode->n;
                while (i > 0 && cmp(rid, newNode->row_ptr[i - 1]) < 0) {
                    newNode->row_ptr[i] = newNode->row_ptr[i - 1];
                    i--;
                }
                newNode->row_ptr[i] = rid;
                newNode->n++;
            }

            if (node->rBro != -1) {
                BNode* rBroNode = (BNode*)get_page(pool, node->rBro);  //* 锁定右兄弟结点块
                rBroNode->lBro = newAddr;   // 连接右兄弟->新结点
                release(pool, node->rBro);  //* 释放右兄弟结点块
            }

            newNode->rBro = node->rBro;
            newNode->lBro = nodeAddr;
            node->rBro = newAddr;
            *newChildAddr_ptr = newAddr;
            *newChildRid_ptr = newNode->row_ptr[0];
            release(pool, nodeAddr);  //* 释放结点块
            release(pool, newAddr);   //* 释放新结点块
            return;                   //! 返回
        }
    }

    // 如果结点是中间结点, 记为N
    else {
        // 选择子树
        size_t child_idx = node->n;
        while (child_idx > 0 && cmp(rid, node->row_ptr[child_idx - 1]) < 0)
            child_idx--;

        RID newChildRid;
        off_t newChildAddr = -1;
        release(pool, nodeAddr);  //* 释放结点块
        b_tree_insert_helper(pool, node->child[child_idx], rid, &newChildRid, &newChildAddr,
                             cmp, insert_handler);  //? 递归向子树插入

        // 如果中间结点需要插入<newChildRid, newChildAddr>
        if (newChildAddr != -1) {
            node = (BNode*)get_page(pool, nodeAddr);  //* 锁定结点块
            // 如果中间结点有空间(通常情况)
            if (node->n < 2 * DEGREE - 1) {
                size_t j = node->n;
                while (j > child_idx) {
                    node->row_ptr[j] = node->row_ptr[j - 1];
                    node->child[j + 1] = node->child[j];
                    j--;
                }
                node->row_ptr[child_idx] = insert_handler(newChildRid);
                node->child[child_idx + 1] = newChildAddr;
                node->n++;
                release(pool, nodeAddr);  //* 释放结点块
                return;                   //! 返回
            }
            // 如果中间结点已满
            else {
                // 将前DEGREE-1个rid, 前DEGREE个addr留在N中
                // 将后DEGREE-1个rid, 后DEGREE个addr移至新结点N2
                off_t lbroAddr = node->lBro;
                off_t rbroAddr = node->rBro;
                off_t newAddr = b_tree_alloc(pool);
                BNode* newNode = (BNode*)get_page(pool, newAddr);  //* 锁定新结点块
                newNode->leaf = 0;                                 // 新结点是中间结点
                newNode->lBro = nodeAddr;
                newNode->rBro = rbroAddr;
                node->rBro = newAddr;
                if (rbroAddr != -1) {
                    BNode* rBroNode = (BNode*)get_page(pool, rbroAddr);  //* 锁定右兄弟结点块
                    rBroNode->lBro = newAddr;  // 连接右兄弟->新结点
                    release(pool, rbroAddr);   //* 释放右兄弟结点块
                }

                for (size_t i = 0; i < DEGREE - 1; i++) {
                    newNode->row_ptr[i] = node->row_ptr[i + DEGREE];  // {0,1}={3,4}
                    newNode->child[i] = node->child[i + DEGREE];      // {0,1}={3,4}
                }
                newNode->child[DEGREE - 1] = node->child[2 * DEGREE - 1];  // {2}={5}
                node->n = DEGREE - 1;
                newNode->n = DEGREE - 1;

                // 如果child_idx<=DEGREE-1, 则向N插入<newChildRid, newChildAddr>
                if (child_idx <= DEGREE - 1) {
                    size_t j = node->n;
                    while (j > child_idx) {
                        node->row_ptr[j] = node->row_ptr[j - 1];
                        node->child[j + 1] = node->child[j];
                        j--;
                    }
                    node->row_ptr[child_idx] = insert_handler(newChildRid);
                    node->child[child_idx + 1] = newChildAddr;
                    node->n++;
                }
                // 否则, 向N2插入<newChildRid, newChildAddr>
                else {
                    size_t n2_child_idx = child_idx - DEGREE;
                    size_t j = newNode->n;
                    while (j > n2_child_idx) {
                        newNode->row_ptr[j] = newNode->row_ptr[j - 1];
                        newNode->child[j + 1] = newNode->child[j];
                        j--;
                    }
                    newNode->row_ptr[n2_child_idx] = insert_handler(newChildRid);
                    newNode->child[n2_child_idx + 1] = newChildAddr;
                    newNode->n++;
                }

                // 将N的最右孩子的rBro置为-1
                BNode* lastChildNode =
                    (BNode*)get_page(pool, node->child[node->n]);  //* 锁定最右孩子结点块
                lastChildNode->rBro = -1;
                release(pool, node->child[node->n]);  //* 释放最右孩子结点块

                // 将N2的最左孩子的lBro置为-1
                BNode* firstChildNode =
                    (BNode*)get_page(pool, newNode->child[0]);  //* 锁定最左孩子结点块
                firstChildNode->lBro = -1;
                release(pool, newNode->child[0]);  //* 释放最左孩子结点块

                node->rBro = newAddr;
                newNode->lBro = nodeAddr;
                release(pool, nodeAddr);  //* 释放结点块
                release(pool, newAddr);   //* 释放新结点块

                off_t tempAddr = newAddr;
                BNode* tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定块
                while (tempNode->leaf == 0) {
                    release(pool, tempAddr);                      //* 释放旧块
                    tempAddr = tempNode->child[0];
                    tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定新块
                }
                *newChildAddr_ptr = newAddr;
                *newChildRid_ptr = tempNode->row_ptr[0];
                release(pool, tempAddr);  //* 释放块
                return;                   //! 返回
            }
        }
    }
}

// 插入<rid.block_addr, rid.idx>
void b_tree_insert(BufferPool* pool,
                   RID rid,
                   b_tree_row_row_cmp_t cmp,
                   b_tree_insert_nonleaf_handler_t insert_handler) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    release(pool, 0);                                   //* 释放控制块
    RID newChildRid;
    off_t newChildAddr = -1;
    b_tree_insert_helper(pool, ctrl->root_node, rid, &newChildRid, &newChildAddr, cmp,
                         insert_handler);

    // 如果根结点需要分裂
    if (newChildAddr != -1) {
        off_t newRootAddr = b_tree_alloc(pool);
        BNode* newRootNode = (BNode*)get_page(pool, newRootAddr);  //* 锁定新根结点块
        newRootNode->leaf = 0;  // 新根结点是中间结点
        newRootNode->lBro = -1;
        newRootNode->rBro = -1;
        newRootNode->child[0] = ctrl->root_node;
        newRootNode->row_ptr[0] = insert_handler(newChildRid);
        newRootNode->child[1] = newChildAddr;
        newRootNode->n = 1;
        ctrl->root_node = newRootAddr;
        release(pool, newRootAddr);  //* 释放新根结点块
    }
}

// ------------------------------------------------

void b_tree_delete_helper(BufferPool* pool,
                          off_t parNodeAddr,
                          off_t nodeAddr,
                          RID rid,
                          off_t* oldChildAddr_ptr,  // 上删的结点地址
                          b_tree_row_row_cmp_t cmp,
                          b_tree_insert_nonleaf_handler_t insert_handler,
                          b_tree_delete_nonleaf_handler_t delete_handler) {
    BNode* node = (BNode*)get_page(pool, nodeAddr);  //* 锁定结点块

    // 如果是叶子结点, 记为L
    if (node->leaf) {
        size_t i = 0;
        while (i < node->n && cmp(rid, node->row_ptr[i]) != 0)
            i++;
        assert(i < node->n);

        for (size_t j = i; j < node->n - 1; j++)
            node->row_ptr[j] = node->row_ptr[j + 1];
        node->n--;

        // 如果非根结点的项数过少, 则需要重分配, 或是合并
        BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
        release(pool, 0);                                   //* 释放控制块
        if (node->n < DEGREE - 1 && nodeAddr != ctrl->root_node) {
            // 如果左兄弟结点有多余项, 则重分配
            if (node->lBro != -1) {
                BNode* lbroNode = (BNode*)get_page(pool, node->lBro);  //* 锁定左兄弟结点块
                release(pool, node->lBro);  //* 释放左兄弟结点块
                if (lbroNode->n >= DEGREE) {
                    // 将lbroNode的最后一项插入为node的第一项
                    for (size_t j = node->n; j > 0; j--)
                        node->row_ptr[j] = node->row_ptr[j - 1];
                    node->row_ptr[0] = lbroNode->row_ptr[lbroNode->n - 1];
                    node->n++;
                    lbroNode->n--;

                    // 更新父结点中node对应的rid
                    BNode* parNode = (BNode*)get_page(pool, parNodeAddr);  //* 锁定父结点块
                    release(pool, parNodeAddr);  //* 释放父结点块
                    size_t k = 0;
                    while (k < parNode->n && parNode->child[k + 1] != nodeAddr)
                        k++;
                    assert(k < parNode->n);
                    parNode->row_ptr[k] = node->row_ptr[0];

                    release(pool, nodeAddr);  //* 释放结点块
                    return;                   //! 返回
                }
            }
            // 如果右兄弟结点有多余项, 则重分配
            if (node->rBro != -1) {
                BNode* rbroNode = (BNode*)get_page(pool, node->rBro);  //* 锁定右兄弟结点块
                release(pool, node->rBro);  //* 释放右兄弟结点块
                if (rbroNode->n >= DEGREE) {
                    // 将rbroNode的第一项插入为node的最后一项
                    node->row_ptr[node->n] = rbroNode->row_ptr[0];
                    for (size_t j = 0; j < rbroNode->n - 1; j++)
                        rbroNode->row_ptr[j] = rbroNode->row_ptr[j + 1];
                    node->n++;
                    rbroNode->n--;

                    // 更新父结点中rBroNode对应的rid
                    BNode* parNode = (BNode*)get_page(pool, parNodeAddr);  //* 锁定父结点块
                    release(pool, parNodeAddr);  //* 释放父结点块
                    size_t k = 0;
                    while (k < parNode->n && parNode->child[k + 1] != node->rBro)
                        k++;
                    assert(k < parNode->n);
                    parNode->row_ptr[k] = rbroNode->row_ptr[0];

                    release(pool, nodeAddr);  //* 释放结点块
                    return;                   //! 返回
                }
            }
            // 将结点块合并到左兄弟结点, 并令oldChildAddr=nodeAddr
            if (node->lBro != -1) {
                off_t lbroAddr = node->lBro;
                off_t rbroAddr = node->rBro;
                BNode* lbroNode = (BNode*)get_page(pool, lbroAddr);  //* 锁定左兄弟结点块
                for (size_t j = 0; j < node->n; j++)
                    lbroNode->row_ptr[lbroNode->n + j] = node->row_ptr[j];
                lbroNode->n += node->n;
                lbroNode->rBro = rbroAddr;
                if (rbroAddr != -1) {
                    BNode* rbroNode = (BNode*)get_page(pool, rbroAddr);  //* 锁定右兄弟结点块
                    rbroNode->lBro = lbroAddr;  // 连接右兄弟->左兄弟
                    release(pool, rbroAddr);    //* 释放右兄弟结点块
                }
                *oldChildAddr_ptr = nodeAddr;
                release(pool, lbroAddr);  //* 释放左兄弟结点块
                release(pool, nodeAddr);  //* 释放结点块
                b_tree_free(pool, nodeAddr);
                return;  //! 返回
            }
            // 将右兄弟结点合并到结点块, 并令oldChildAddr=rbroAddr
            if (node->rBro != -1) {
                off_t lbroAddr = node->lBro;
                off_t rbroAddr = node->rBro;
                BNode* rbroNode = (BNode*)get_page(pool, rbroAddr);  //* 锁定右兄弟结点块
                release(pool, rbroAddr);  //* 释放右兄弟结点块
                for (size_t j = 0; j < rbroNode->n; j++)
                    node->row_ptr[node->n + j] = rbroNode->row_ptr[j];
                node->n += rbroNode->n;
                node->rBro = rbroNode->rBro;
                if (rbroNode->rBro != -1) {
                    BNode* rrbroNode =
                        (BNode*)get_page(pool, rbroNode->rBro);  //* 锁定右右兄弟结点块
                    rrbroNode->lBro = nodeAddr;     // 连接右右兄弟->当前结点
                    release(pool, rbroNode->rBro);  //* 释放右右兄弟结点块
                }
                *oldChildAddr_ptr = rbroAddr;
                release(pool, nodeAddr);  //* 释放结点块
                b_tree_free(pool, rbroAddr);
                return;  //! 返回
            }
        }

        release(pool, nodeAddr);  //* 释放结点块
        return;                   //! 返回

    }

    // 如果是中间结点, 记为N
    else {
        // 选择子树
        size_t child_idx = node->n;
        while (child_idx > 0 && cmp(rid, node->row_ptr[child_idx - 1]) < 0)
            child_idx--;
        off_t oldChildAddr = -1;
        release(pool, nodeAddr);  //* 释放结点块
        b_tree_delete_helper(pool, nodeAddr, node->child[child_idx], rid, &oldChildAddr, cmp,
                             insert_handler, delete_handler);

        // 如果中间结点需要删除<oldChildAddr>
        if (oldChildAddr != -1) {
            node = (BNode*)get_page(pool, nodeAddr);  //* 锁定结点块

            size_t i = 0;
            while (i < node->n && node->child[i + 1] != oldChildAddr)
                i++;
            assert(i < node->n);

            for (size_t j = i; j < node->n - 1; j++) {
                node->row_ptr[j] = node->row_ptr[j + 1];
                node->child[j + 1] = node->child[j + 2];
            }
            node->n--;

            BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
            release(pool, 0);                                   //* 释放控制块

            // 如果根节点作为中间结点被清空
            if (node->n == 0 && nodeAddr == ctrl->root_node) {
                *oldChildAddr_ptr = node->child[0];
                release(pool, nodeAddr);  //* 释放结点块
                return;                   //! 返回
            }

            // 如果非根结点的项数过少, 则需要重分配, 或是合并
            if (node->n < DEGREE - 1 && nodeAddr != ctrl->root_node) {
                // 如果左兄弟结点有多余项, 则重分配
                if (node->lBro != -1) {
                    BNode* lbroNode = (BNode*)get_page(pool, node->lBro);  //* 锁定左兄弟结点块
                    release(pool, node->lBro);  //* 释放左兄弟结点块
                    if (lbroNode->n >= DEGREE) {
                        // 将lbroNode的最后一项插入为node的第一项
                        for (size_t j = node->n; j > 0; j--) {
                            node->row_ptr[j] = node->row_ptr[j - 1];
                            node->child[j + 1] = node->child[j];
                        }
                        node->child[1] = node->child[0];
                        node->child[0] = lbroNode->child[lbroNode->n];

                        // 找到node->child[1]对应子树中的最小键
                        off_t tempAddr = node->child[1];
                        BNode* tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定块
                        while (tempNode->leaf == 0) {
                            release(pool, tempAddr);                      //* 释放旧块
                            tempAddr = tempNode->child[0];
                            tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定新块
                        }
                        release(pool, tempAddr);  //* 释放块
                        node->row_ptr[0] = tempNode->row_ptr[0];
                        node->n++;
                        lbroNode->n--;

                        // 更新lbroNode最右孩子的rBro=-1
                        BNode* lastChildNode = (BNode*)get_page(
                            pool, lbroNode->child[lbroNode->n]);  //* 锁定最右孩子结点块
                        lastChildNode->rBro = -1;
                        release(pool, lbroNode->child[lbroNode->n]);  //* 释放最右孩子结点块

                        // 更新node最左孩子的lBro=-1,rBro=[1]
                        BNode* firstChildNode =
                            (BNode*)get_page(pool, node->child[0]);  //* 锁定最左孩子结点块
                        firstChildNode->lBro = -1;
                        if (1 <= node->n)
                            firstChildNode->rBro = node->child[1];
                        else
                            firstChildNode->rBro = -1;
                        release(pool, node->child[0]);  //* 释放最左孩子结点块

                        // 如果[1]存在, 则更新[1]的lBro=[0]
                        if (1 <= node->n) {
                            BNode* secondChildNode =
                                (BNode*)get_page(pool, node->child[1]);  //* 锁定第二孩子结点块
                            secondChildNode->lBro = node->child[0];
                            release(pool, node->child[1]);  //* 释放第二孩子结点块
                        }

                        // 更新父结点中node对应的rid
                        BNode* parNode = (BNode*)get_page(pool, parNodeAddr);  //* 锁定父结点块
                        release(pool, parNodeAddr);  //* 释放父结点块
                        size_t k = 0;
                        while (k < parNode->n && parNode->child[k + 1] != nodeAddr)
                            k++;
                        assert(k < parNode->n);
                        // 找到node对应子树的最小键
                        tempAddr = node->child[0];
                        tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定块
                        while (tempNode->leaf == 0) {
                            release(pool, tempAddr);                      //* 释放旧块
                            tempAddr = tempNode->child[0];
                            tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定新块
                        }
                        release(pool, tempAddr);  //* 释放块
                        parNode->row_ptr[k] = tempNode->row_ptr[0];

                        release(pool, nodeAddr);  //* 释放结点块
                        return;                   //! 返回
                    }
                }
                // 如果右兄弟结点有多余项, 则重分配
                if (node->rBro != -1) {
                    BNode* rbroNode = (BNode*)get_page(pool, node->rBro);  //* 锁定右兄弟结点块
                    release(pool, node->rBro);  //* 释放右兄弟结点块
                    if (rbroNode->n >= DEGREE) {
                        // 将rbroNode的第一项插入为node的最后一项
                        node->child[node->n + 1] = rbroNode->child[0];
                        // 找到node->child[node->n + 1]对应子树的最小键
                        off_t tempAddr = node->child[node->n + 1];
                        BNode* tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定块
                        while (tempNode->leaf == 0) {
                            release(pool, tempAddr);                      //* 释放旧块
                            tempAddr = tempNode->child[0];
                            tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定新块
                        }
                        release(pool, tempAddr);  //* 释放块
                        node->row_ptr[node->n] = tempNode->row_ptr[0];

                        for (size_t j = 0; j < rbroNode->n - 1; j++) {
                            rbroNode->row_ptr[j] = rbroNode->row_ptr[j + 1];
                            rbroNode->child[j] = rbroNode->child[j + 1];
                        }
                        rbroNode->child[rbroNode->n - 1] = rbroNode->child[rbroNode->n];
                        node->n++;
                        rbroNode->n--;

                        // 更新rbroNode最左孩子的lBro=-1
                        BNode* firstChildNode =
                            (BNode*)get_page(pool, rbroNode->child[0]);  //* 锁定最左孩子结点块
                        firstChildNode->lBro = -1;
                        release(pool, rbroNode->child[0]);  //* 释放最左孩子结点块

                        // 更新node最右孩子的rBro=-1,lBro=[n-1]
                        BNode* lastChildNode = (BNode*)get_page(
                            pool, node->child[node->n]);  //* 锁定最右孩子结点块
                        lastChildNode->rBro = -1;
                        if (node->n - 1 >= 0)
                            lastChildNode->lBro = node->child[node->n - 1];
                        else
                            lastChildNode->lBro = -1;
                        release(pool, node->child[node->n]);  //* 释放最右孩子结点块

                        // 如果存在, 更新node[n-1]的rBro=[n]
                        if (node->n - 1 >= 0) {
                            BNode* lastChildNode = (BNode*)get_page(
                                pool, node->child[node->n - 1]);  //* 锁定最右孩子结点块
                            lastChildNode->rBro = node->child[node->n];
                            release(pool, node->child[node->n - 1]);  //* 释放最右孩子结点块
                        }

                        // 更新父结点中rBroNode对应的rid
                        BNode* parNode = (BNode*)get_page(pool, parNodeAddr);  //* 锁定父结点块
                        release(pool, parNodeAddr);  //* 释放父结点块
                        size_t k = 0;
                        while (k < parNode->n && parNode->child[k + 1] != node->rBro)
                            k++;
                        assert(k < parNode->n);
                        // 找到rbroNode对应子树的最小键
                        tempAddr = rbroNode->child[0];
                        tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定块
                        while (tempNode->leaf == 0) {
                            release(pool, tempAddr);                      //* 释放旧块
                            tempAddr = tempNode->child[0];
                            tempNode = (BNode*)get_page(pool, tempAddr);  //* 锁定新块
                        }
                        release(pool, tempAddr);  //* 释放块
                        parNode->row_ptr[k] = tempNode->row_ptr[0];

                        release(pool, nodeAddr);  //* 释放结点块
                        return;                   //! 返回
                    }
                }

                // 下拉分割码, 将结点块合并到左兄弟结点, 并令oldChildAddr=nodeAddr
                if (node->lBro != -1) {
                    off_t lbroAddr = node->lBro;
                    off_t rbroAddr = node->rBro;
                    BNode* lbroNode = (BNode*)get_page(pool, lbroAddr);  //* 锁定左兄弟结点块
                    BNode* parNode = (BNode*)get_page(pool, parNodeAddr);  //* 锁定父结点块
                    release(pool, parNodeAddr);  //* 释放父结点块

                    // 获取父结点中node对应的rid, 插入到lbroNode的最后一项
                    size_t k = 0;
                    while (k < parNode->n && parNode->child[k + 1] != nodeAddr)
                        k++;
                    assert(k < parNode->n);
                    lbroNode->row_ptr[lbroNode->n] = parNode->row_ptr[k];
                    lbroNode->n++;

                    // 将node的所有项插入到lbroNode的后面
                    lbroNode->child[lbroNode->n] = node->child[0];
                    for (size_t j = 0; j < node->n; j++) {
                        lbroNode->row_ptr[lbroNode->n + j] = node->row_ptr[j];
                        lbroNode->child[lbroNode->n + j + 1] = node->child[j + 1];
                    }
                    lbroNode->n += node->n;
                    lbroNode->rBro = rbroAddr;

                    // 更新左右孩子的lBro,rBro
                    for (size_t j = 0; j <= lbroNode->n; j++) {
                        BNode* childNode =
                            (BNode*)get_page(pool, lbroNode->child[j]);  //* 锁定孩子结点块
                        if (j != 0)
                            childNode->lBro = lbroNode->child[j - 1];
                        else
                            childNode->lBro = -1;
                        if (j != lbroNode->n)
                            childNode->rBro = lbroNode->child[j + 1];
                        else
                            childNode->rBro = -1;
                        release(pool, lbroNode->child[j]);  //* 释放孩子结点块
                    }

                    if (rbroAddr != -1) {
                        BNode* rbroNode =
                            (BNode*)get_page(pool, rbroAddr);  //* 锁定右兄弟结点块
                        rbroNode->lBro = lbroAddr;             // 连接右兄弟->左兄弟
                        release(pool, rbroAddr);               //* 释放右兄弟结点块
                    }
                    *oldChildAddr_ptr = nodeAddr;
                    release(pool, lbroAddr);  //* 释放左兄弟结点块
                    release(pool, nodeAddr);  //* 释放结点块
                    b_tree_free(pool, nodeAddr);
                    return;  //! 返回
                }

                // 下拉分割码, 将右兄弟结点合并到结点块, 并令oldChildAddr=node->rBro
                if (node->rBro != -1) {
                    off_t lbroAddr = node->lBro;
                    off_t rbroAddr = node->rBro;
                    BNode* rbroNode = (BNode*)get_page(pool, rbroAddr);  //* 锁定右兄弟结点块
                    BNode* parNode = (BNode*)get_page(pool, parNodeAddr);  //* 锁定父结点块
                    release(pool, parNodeAddr);  //* 释放父结点块

                    // 获取父结点中rBroNode对应的rid, 插入到node的最后一项
                    size_t k = 0;
                    while (k < parNode->n && parNode->child[k + 1] != rbroAddr)
                        k++;
                    assert(k < parNode->n);
                    node->row_ptr[node->n] = parNode->row_ptr[k];
                    node->n++;

                    // 将rbroNode的所有项插入到node的后面
                    node->child[node->n] = rbroNode->child[0];
                    for (size_t j = 0; j < rbroNode->n; j++) {
                        node->row_ptr[node->n + j] = rbroNode->row_ptr[j];
                        node->child[node->n + j + 1] = rbroNode->child[j + 1];
                    }
                    node->n += rbroNode->n;
                    node->rBro = rbroNode->rBro;

                    // 更新左右孩子的lBro,rBro
                    for (size_t j = 0; j <= node->n; j++) {
                        BNode* childNode =
                            (BNode*)get_page(pool, node->child[j]);  //* 锁定孩子结点块
                        if (j != 0)
                            childNode->lBro = node->child[j - 1];
                        else
                            childNode->lBro = -1;
                        if (j != node->n)
                            childNode->rBro = node->child[j + 1];
                        else
                            childNode->rBro = -1;
                        release(pool, node->child[j]);  //* 释放孩子结点块
                    }

                    if (rbroNode->rBro != -1) {
                        BNode* rrbroNode =
                            (BNode*)get_page(pool, rbroNode->rBro);  //* 锁定右右兄弟结点块
                        rrbroNode->lBro = nodeAddr;     // 连接右右兄弟->当前结点
                        release(pool, rbroNode->rBro);  //* 释放右右兄弟结点块
                    }
                    *oldChildAddr_ptr = rbroAddr;
                    release(pool, rbroAddr);  //* 释放右兄弟结点块
                    release(pool, nodeAddr);  //* 释放结点块
                    b_tree_free(pool, rbroAddr);
                    return;  //! 返回
                }
            }

            // 直接删除
            release(pool, nodeAddr);  //* 释放结点块
            return;                   //! 返回
        }
    }
}

// 删除key=rid.block_addr对应的结点项
void b_tree_delete(BufferPool* pool,
                   RID rid,
                   b_tree_row_row_cmp_t cmp,
                   b_tree_insert_nonleaf_handler_t insert_handler,
                   b_tree_delete_nonleaf_handler_t delete_handler) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    release(pool, 0);                                   //* 释放控制块
    off_t oldChildAddr = -1;
    b_tree_delete_helper(pool, -1, ctrl->root_node, rid, &oldChildAddr, cmp, insert_handler,
                         delete_handler);

    // 如果中间结点作为根结点被清空
    if (oldChildAddr != -1) {
        BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
        ctrl->root_node = oldChildAddr;                     // 更新根结点
        release(pool, 0);                                   //* 释放控制块
    }
}

// --------------------------------------------------------------------------

void b_tree_print_helper(BufferPool* pool, off_t nodeAddr, int depth) {
    BNode* node = (BNode*)get_page(pool, nodeAddr);  //* 锁定结点块
    release(pool, nodeAddr);  //* 释放结点块


    for (int i = 0; i < depth; i++)
        printf("  ");
    printf("<%ld-%d-%ld>Node %ld: ", node->lBro, depth, node->rBro, nodeAddr);
    for (size_t i = 0; i < node->n; i++) {
        if (node->leaf == 0)
            printf("(%ld) ", node->child[i]);
        else
            printf("(0) ");
        printf("%ld ", get_rid_block_addr(node->row_ptr[i]));

        if (i != (node->n - 1))
            assert(get_rid_block_addr(node->row_ptr[i]) <
                   get_rid_block_addr(node->row_ptr[i + 1]));
    }
    if (node->leaf == 0)
        printf("(%ld)\n", node->child[node->n]);
    else
        printf("(0)\n");

    if (node->leaf)
        return;

    for (size_t i = 0; i <= node->n; i++) {
        BNode* childNode = (BNode*)get_page(pool, node->child[i]);  //* 锁定子结点块
        if (i == 0) {
            assert(get_rid_block_addr(childNode->row_ptr[0]) <
                   get_rid_block_addr(node->row_ptr[0]));
            assert(childNode->lBro == -1);
            assert(childNode->rBro == node->child[i + 1]);
        } else if (i == node->n) {
            assert(get_rid_block_addr(childNode->row_ptr[0]) >=
                   get_rid_block_addr(node->row_ptr[i - 1]));
            assert(childNode->lBro == node->child[i - 1]);
            assert(childNode->rBro == -1);
        } else {
            assert(get_rid_block_addr(childNode->row_ptr[0]) >=
                   get_rid_block_addr(node->row_ptr[i - 1]));
            assert(childNode->lBro == node->child[i - 1]);
            assert(childNode->rBro == node->child[i + 1]);
        }
        release(pool, node->child[i]);  //* 释放子结点块
    }

    for (size_t i = 0; i <= node->n; i++) {
        b_tree_print_helper(pool, node->child[i], depth + 1);
    }
}

void b_tree_print(BufferPool* pool) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);  //* 锁定控制块
    release(pool, 0);                                   //* 释放控制块

    printf("------------------------------begin\n");
    b_tree_print_helper(pool, ctrl->root_node, 0);
    printf("------------------------------end\n\n");
}