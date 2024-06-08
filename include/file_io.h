#ifndef _FILE_IO_H
#define _FILE_IO_H

#include <stdio.h>
#include <sys/types.h>

#define PAGE_SIZE 128
#define PAGE_MASK (PAGE_SIZE - 1)

typedef struct {
    FILE* fp;      // 文件指针
    off_t length;  // 文件长度
} FileInfo;

typedef struct {
    char data[PAGE_SIZE];
} Page;

typedef enum {
    FILE_OPEN,          // 文件存在并且成功打开
    FILE_CREAT,         // 文件不存在但成功创建
    FILE_IO_SUCCESS,    // 打开文件成功
    FILE_IO_FAILED,     // 打开文件失败
    INVALID_LEN,        // 文件长度不是 PAGE_SIZE 的整数倍
    INVALID_ADDR,       // 地址位置不是 PAGE_SIZE 的整数倍
    ADDR_OUT_OF_RANGE,  // 地址位置超出文件范围
} FileIOResult;

// 打开文件，如果文件不存在则创建
FileIOResult open_file(FileInfo* file, const char* filename);

// 关闭文件
FileIOResult close_file(FileInfo* file);

// 读入位于addr的page, 若addr==文件大小，则追加新的页
FileIOResult read_page(Page* page, FileInfo* file, off_t addr);

// 将page写出至addr
FileIOResult write_page(const Page* page, FileInfo* file, off_t addr);

#endif /* _FILE_IO_H */