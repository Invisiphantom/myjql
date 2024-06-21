#ifndef _B_TREE_H
#define _B_TREE_H

#include "table.h"

// 每页一个结点
#define DEGREE                                                                             \
    (((PAGE_SIZE) - sizeof(size_t) - 3 * sizeof(off_t) - sizeof(char) + sizeof(RID)) / 2 / \
     (sizeof(off_t) + sizeof(RID)))

// B树结点 (120bit)
typedef struct {
    size_t n;
    off_t next;                   // 下一个空闲块
    off_t lBro;                   // 左兄弟
    off_t rBro;                   // 右兄弟
    off_t child[2 * DEGREE];      // 每个结点6个孩子
    RID row_ptr[2 * DEGREE - 1];  // 每个结点5项
    char leaf;                    // 是否是叶子结点 (0:中间, 1:叶子)
} BNode;

// B树控制块
typedef struct {
    off_t root_node;
    off_t free_node_head;
} BCtrlBlock;

typedef int (*b_tree_row_row_cmp_t)(RID, RID);
typedef int (*b_tree_ptr_row_cmp_t)(void*, size_t, RID);

// 中间结点: node->row_ptr[k] = insert_handler(rid);
typedef RID (*b_tree_insert_nonleaf_handler_t)(RID rid);

// 中间结点: delete_handler(node->row_ptr[k]);
typedef void (*b_tree_delete_nonleaf_handler_t)(RID rid);

void b_tree_init(const char* filename, BufferPool* pool);

void b_tree_close(BufferPool* pool);

// 查找key=rid.block_addr对应的结点项
RID b_tree_search(BufferPool* pool, void* key, size_t size, b_tree_ptr_row_cmp_t cmp);

// 插入<rid.block_addr, rid.idx>
void b_tree_insert(BufferPool* pool,
                   RID rid,
                   b_tree_row_row_cmp_t cmp,
                   b_tree_insert_nonleaf_handler_t insert_handler);

// 删除key=rid.block_addr对应的结点项
void b_tree_delete(BufferPool* pool,
                   RID rid,
                   b_tree_row_row_cmp_t cmp,
                   b_tree_insert_nonleaf_handler_t insert_handler,
                   b_tree_delete_nonleaf_handler_t delete_handler);

#endif