//
// Created by 徐辰 on 2017/1/17.
//

#pragma once
#ifndef SLAB_H_INCLUDED
#define SLAB_H_INCLUDED

#include <algorithm>
#include <array>
#include <atomic>

// O(1) lock-free slab allocator for fixed size objects
template<std::size_t N>
struct slab {
    // At least sizeof(void *), align with sizeof(void *)
    static constexpr std::size_t element_size =
            (std::max(N, sizeof(void *)) + sizeof(void *) - 1) / sizeof(void *) * sizeof(void *);
    typedef std::array<uint8_t, element_size> block_t;
    union slot {
        slot *next;
        block_t block;
    };

    slab(std::size_t slots) : capacity_(slots) {
        // Chain all slots into a free list
        data = new slot[slots];
        for (std::size_t i = 0; i < slots - 1; i++) {
            data[i].next = &data[i + 1];
        }
        head = data;
    }

    ~slab() {
        delete[] data;
    }

    slab(const slab &) = delete;

    slab(slab &&) = delete;

    void *allocate() {
        auto p = head.load(std::memory_order_relaxed);
        if (!p) return nullptr;
        while (head.compare_exchange_weak(p, p->next, std::memory_order_release,
                                          std::memory_order_relaxed));
        return p;
    }

    void deallocate(void *p) {
        // P is not nullptr
        assert(p != nullptr);
        // Make sure p is in this slot memory range
        assert(uintptr_t(p) >= uintptr_t(data) && uintptr_t(p) < uintptr_t(data) + (capacity_ * N));
        // Make sure p is pointing to one slot
        assert((uintptr_t(p) - uintptr_t(data)) % element_size == 0);

        slot *new_head = static_cast<slot *>(p);
        new_head->next = head.load(std::memory_order_relaxed);
        while (!head.compare_exchange_weak(new_head->next, new_head, std::memory_order_release,
                                           std::memory_order_relaxed));
    }

    std::size_t capacity() const {
        return capacity_;
    }

    const std::size_t capacity_;
    std::atomic<slot *> head;
    slot *data;
};


#endif // !defined(HUG_SLAB_H_INCLUDED)
