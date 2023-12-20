#ifndef STM_MEMORY_HPP
#define STM_MEMORY_HPP


//region includes

#include <atomic>

#include "lock.hpp"
#include "macros.hpp"
#include "config.hpp"

//endregion

//region defines

#define MEMORY_BLOCK_SIZE           (1ull << VIRTUAL_ADDR_OFFSET_BITS)
#define MEMORY_BLOCK_COUNT          (1ull << VIRTUAL_ADDR_BLOCK_BITS)
#define VIRTUAL_ADDR_MAX_OFFSET     (MEMORY_BLOCK_SIZE - 1)
#define VIRTUAL_ADDR_MAX_BLOCK      (MEMORY_BLOCK_COUNT - 1)

#define START_VIRTUAL_ADDR \
    (1ull << (VIRTUAL_ADDR_BLOCK_BITS + VIRTUAL_ADDR_OFFSET_BITS + ALIGNMENT_BITS))

#define VIRTUAL_ADDR_TO_OFFSET(virtual_addr) \
    (((virtual_addr) >> ALIGNMENT_BITS) & ~(~0ull << VIRTUAL_ADDR_OFFSET_BITS))

#define VIRTUAL_ADDR_TO_BLOCK(virtual_addr) \
    (((virtual_addr) >> (ALIGNMENT_BITS+VIRTUAL_ADDR_OFFSET_BITS)) & ~(~0ull << VIRTUAL_ADDR_BLOCK_BITS))

#define BLOCK_TO_VIRTUAL_ADDR(block_idx) \
    (START_VIRTUAL_ADDR | (block_idx & VIRTUAL_ADDR_MAX_BLOCK) << (ALIGNMENT_BITS+VIRTUAL_ADDR_OFFSET_BITS))

//endregion


namespace stm {

typedef uint64_t virtual_addr_t;

typedef std::atomic<uint64_t> clock_t;


struct memory_word_t {
    lock_t lock;
    uint64_t data;

    /**
     * Default constructor.
     */
    memory_word_t() : lock(), data(0) {}

//    /**
//     * Copy constructor.
//     */
    memory_word_t(const memory_word_t &other) : lock(other.lock), data(other.data) {}

};


struct memory_region_t {

    clock_t clock = 0;
    memory_word_t words[MEMORY_BLOCK_COUNT][MEMORY_BLOCK_SIZE];
    std::atomic<uint64_t> allocated_block_cnt = 1;
    size_t size;

    memory_word_t &operator[](virtual_addr_t virtual_addr) {
        // todo add bound checks
        auto offset = VIRTUAL_ADDR_TO_OFFSET(virtual_addr);
        auto block = VIRTUAL_ADDR_TO_BLOCK(virtual_addr);
        return words[block][offset];
    }

    always_inline virtual_addr_t alloc_block() {
        auto new_block_idx = allocated_block_cnt.fetch_add(1, std::memory_order_relaxed);
        return BLOCK_TO_VIRTUAL_ADDR(new_block_idx);
    }

};


}


#endif
