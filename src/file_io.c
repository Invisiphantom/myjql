#define _CRT_SECURE_NO_WARNINGS

#include "file_io.h"

#include <stdio.h>
#include <assert.h>

FileIOResult open_file(FileInfo* file, const char* filename) {
    int res;
    if (file->fp = fopen(filename, "rb+")) {
        /* 文件存在并且成功打开 */
        res = FILE_OPEN;
    } else if (file->fp = fopen(filename, "wb+")) {
        /* 文件不存在但成功创建 */
        res = FILE_CREAT;
    } else {
        return FILE_IO_FAILED;
    }
    
    // 文件指针移动到末尾
    if (fseek(file->fp, 0, SEEK_END)) {
        return FILE_IO_FAILED;
    }
    // 获取文件长度
    file->length = ftell(file->fp);
    // 文件长度是 PAGE_SIZE 的整数倍
    if (file->length & PAGE_MASK) {
        close_file(file);
        return INVALID_LEN;
    }
    return res;
}

FileIOResult close_file(FileInfo* file) {
    fclose(file->fp);
    file->fp = NULL;
    file->length = 0;
    return FILE_IO_SUCCESS;
}

// 读入位于addr的page
FileIOResult read_page(Page* page, FileInfo* file, off_t addr) {
    // 如果addr==文件大小，则追加新的页
    if(file->length & PAGE_MASK) {
        fprintf(stderr, "read_page: file->length未对齐 %ld\n", file->length);
        assert(0);
    }
    if (addr == file->length) {
        file->length += PAGE_SIZE;
        if(file->length == 49495)
            assert(0);
    }
    // 地址是 PAGE_SIZE 的整数倍
    if (addr & PAGE_MASK) {
        printf("read_page: addr未对齐: %ld\n", addr);
        assert(0);
        return INVALID_ADDR;
    }
    // 地址在文件范围内
    if (addr < 0 || addr >= file->length) {
        printf("read_page: addr超出范围: %ld\n", addr);
        assert(0);
        return ADDR_OUT_OF_RANGE;
    }
    // 移动文件指针到addr
    if (fseek(file->fp, (long)addr, SEEK_SET)) {
        return FILE_IO_FAILED;
    }
    // 读取 PAGE_SIZE 字节
    size_t bytes_read = fread(page, PAGE_SIZE, 1, file->fp);
    // 读取失败
    if (bytes_read != 1) {
        return FILE_IO_FAILED;
    }
    return FILE_IO_SUCCESS;
}

// 将page写出至addr，若addr==文件大小，则追加新的页
FileIOResult write_page(const Page* page, FileInfo* file, off_t addr) {
    // 如果addr==文件大小，则追加新的页
    if (addr == file->length) {
        file->length += PAGE_SIZE;
    }
    // 地址是 PAGE_SIZE 的整数倍
    if (addr & PAGE_MASK) {
        printf("write_page: addr未对齐: %ld\n", addr);
        assert(0);
        return INVALID_ADDR;
    }
    // 地址在文件范围内
    if (addr < 0 || addr > file->length) {
        printf("write_page: addr超出范围: %ld\n", addr);
        assert(0);
        return ADDR_OUT_OF_RANGE;
    }
    // 移动文件指针到addr
    if (fseek(file->fp, (long)addr, SEEK_SET)) {
        return FILE_IO_FAILED;
    }
    // 写入 PAGE_SIZE 字节
    size_t bytes_written = fwrite(page, PAGE_SIZE, 1, file->fp);
    if (bytes_written != 1) {
        return FILE_IO_FAILED;
    }
    return FILE_IO_SUCCESS;
}
