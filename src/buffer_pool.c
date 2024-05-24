#include "buffer_pool.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

void init_buffer_pool(const char *filename, BufferPool *pool) {
}

void close_buffer_pool(BufferPool *pool) {
}

Page *get_page(BufferPool *pool, off_t addr) {
}

void release(BufferPool *pool, off_t addr) {
}

/* void print_buffer_pool(BufferPool *pool) {
} */

/* void validate_buffer_pool(BufferPool *pool) {
} */
