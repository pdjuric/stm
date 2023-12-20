
#include "config.hpp"
#include "tm.hpp"
#include "tx.hpp"


static stm::memory_region_t memory_region;
static thread_local stm::update_tx_t tl_update_tx;
static thread_local stm::readonly_tx_t tl_readonly_tx;


tx_t tm_begin(shared_t unused(shared), bool is_ro) noexcept {
    if (is_ro) {
        tl_readonly_tx.setStartTime(memory_region.clock);
        return (tx_t) &tl_readonly_tx;
    } else {
        tl_update_tx.setStartTime(memory_region.clock);
        return SET_BIT((tx_t) &tl_readonly_tx, 0);
    }
}

bool tm_end(shared_t unused(shared), tx_t unused(tx)) noexcept {
    if (IS_RESET_BIT(tx, 0)) return true;
    else return tl_update_tx.commit(memory_region);
}

bool tm_read(shared_t unused(shared), tx_t tx, void const* source, size_t size, void* target) noexcept {
    auto virtual_addr = (stm::virtual_addr_t) source;
    auto dst = (uint64_t*) target;

    if (IS_SET_BIT(tx, 0))
        return tl_update_tx.read(memory_region, virtual_addr, size, dst);
    else
        return tl_readonly_tx.read(memory_region, virtual_addr, size, dst);

}

bool tm_write(shared_t unused(shared), tx_t unused(tx), void const* source, size_t size, void* target) noexcept {
    auto virtual_addr = (stm::virtual_addr_t) target;
    auto src = (uint64_t*) source;
    return tl_update_tx.write(memory_region, src, size, virtual_addr);
}


Alloc tm_alloc(shared_t unused(shared), tx_t unused(tx), size_t unused(size), void** target) noexcept {
    *target = (void *)(memory_region.alloc_block());
    return Alloc::success;
}


bool tm_free(shared_t unused(shared), tx_t unused(tx), void* unused(target)) noexcept {
    // todo try to do nothing
    return true;
}

shared_t tm_create(size_t size, size_t unused(align)) noexcept {
    memory_region.size = size;
    return &memory_region;
}
void tm_destroy(shared_t unused(shared)) noexcept {
}


void* tm_start(shared_t unused(shared)) noexcept {
    return (void*) START_VIRTUAL_ADDR;
}


size_t tm_size(shared_t unused(shared)) noexcept {
    return memory_region.size;
}


size_t tm_align(shared_t unused(shared)) noexcept {
    return ALIGNMENT;
}