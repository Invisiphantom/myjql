#ifndef _FILE_IO_H
#define _FILE_IO_H

#include <stdio.h>
#include <sys/types.h>

#define PAGE_SIZE 128
// #define PAGE_SIZE 64

#define PAGE_MASK (PAGE_SIZE - 1)

/* off_t {long int} */
#define FORMAT_OFF_T "%ld"
/* size_t {long unsigned int} */
#define FORMAT_SIZE_T "%ld"

typedef struct {
    FILE* fp;      // 文件指针
    off_t length;  // 文件长度
} FileInfo;

typedef struct {
    char data[PAGE_SIZE]; // 占位对齐 PAGE_SIZE
} Page;

typedef enum {
    FILE_IO_SUCCESS,    // 打开文件成功
    FILE_IO_FAILED,     // 打开文件失败
    INVALID_LEN,        // 文件长度不是 PAGE_SIZE 的整数倍
    INVALID_ADDR,       // 地址位置不是 PAGE_SIZE 的整数倍
    ADDR_OUT_OF_RANGE,  // 地址位置超出文件范围
} FileIOResult;

FileIOResult open_file(FileInfo* file, const char* filename);

FileIOResult close_file(FileInfo* file);

// 读入位于addr的page
FileIOResult read_page(Page* page, const FileInfo* file, off_t addr);

// 将page写出至addr，若addr==文件大小，则追加新的页
FileIOResult write_page(const Page* page, FileInfo* file, off_t addr);

#endif /* _FILE_IO_H */