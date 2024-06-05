#define _CRT_SECURE_NO_WARNINGS

#include "file_io.h"

#include <stdio.h>

FileIOResult open_file(FileInfo* file, const char* filename) {
    if (file->fp = fopen(filename, "rb+")) {
        /* 文件存在并且成功打开 */
    } else if (file->fp = fopen(filename, "wb+")) {
        /* 文件不存在但成功创建 */
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
    return FILE_IO_SUCCESS;
}

FileIOResult close_file(FileInfo* file) {
    fclose(file->fp);
    file->fp = NULL;
    file->length = 0;
    return FILE_IO_SUCCESS;
}

// 读入位于addr的page
FileIOResult read_page(Page* page, const FileInfo* file, off_t addr) {
    // 地址是 PAGE_SIZE 的整数倍
    if (addr & PAGE_MASK) {
        return INVALID_ADDR;
    }
    // 地址在文件范围内
    if (addr < 0 || addr >= file->length) {
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
    // 地址是 PAGE_SIZE 的整数倍
    if (addr & PAGE_MASK) {
        return INVALID_ADDR;
    }
    // 地址在文件范围内
    if (addr < 0 || addr > file->length) {
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
    // 如果addr==文件大小，则追加新的页
    if (addr == file->length) { /* append new page in the end*/
        file->length += PAGE_SIZE;
    }
    return FILE_IO_SUCCESS;
}
