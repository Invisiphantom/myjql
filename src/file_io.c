#include "file_io.h"

#include <stdio.h>
#include <assert.h>

FileIOResult open_file(FileInfo* file, const char* filename) {
    FileIOResult res;
    if (file->fp = fopen(filename, "rb+"))
        res = FILE_OPEN;
    else if (file->fp = fopen(filename, "wb+"))
        res = FILE_CREAT;
    else
        return FILE_IO_FAILED;

    // 文件指针移动到末尾
    if (fseek(file->fp, 0, SEEK_END))
        return FILE_IO_FAILED;

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
    if (addr == file->length)
        file->length += PAGE_SIZE;

    if (addr & PAGE_MASK) {
        fprintf(stderr, "read_page: addr未对齐: %ld\n", addr);
        assert(0);
        return INVALID_ADDR;
    }
    if (addr < 0 || addr >= file->length) {
        fprintf(stderr, "read_page: addr超出范围: %ld\n", addr);
        assert(0);
        return ADDR_OUT_OF_RANGE;
    }
    // 移动文件指针到addr
    if (fseek(file->fp, (long)addr, SEEK_SET))
        return FILE_IO_FAILED;

    // 读取 PAGE_SIZE 字节
    size_t bytes_read = fread(page, PAGE_SIZE, 1, file->fp);
    if (bytes_read != 1)  // 读取失败
        return FILE_IO_FAILED;

    return FILE_IO_SUCCESS;
}

// 将page写出至addr
FileIOResult write_page(const Page* page, FileInfo* file, off_t addr) {
    // 如果addr==文件大小，则追加新的页
    if (addr == file->length)
        file->length += PAGE_SIZE;

    if (addr & PAGE_MASK) {
        printf("write_page: addr未对齐: %ld\n", addr);
        assert(0);
        return INVALID_ADDR;
    }
    if (addr < 0 || addr > file->length) {
        printf("write_page: addr超出范围: %ld\n", addr);
        assert(0);
        return ADDR_OUT_OF_RANGE;
    }

    // 移动文件指针到addr
    if (fseek(file->fp, (long)addr, SEEK_SET))
        return FILE_IO_FAILED;

    // 写入 PAGE_SIZE 字节
    size_t bytes_written = fwrite(page, PAGE_SIZE, 1, file->fp);
    if (bytes_written != 1)  // 写入失败
        return FILE_IO_FAILED;

    return FILE_IO_SUCCESS;
}
