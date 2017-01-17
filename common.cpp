#include "common.h"
#include "slab.h"

std::mt19937 &rng() {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
}

static slab<buf_size> buffer_slab(1024);

void *allocate_buffer() {
    return buffer_slab.allocate();
}

void deallocate_buffer(void *p) {
    buffer_slab.deallocate(p);
}