#ifndef STM_LOCK_HPP
#define STM_LOCK_HPP


//region includes

#include <atomic>

#include "macros.hpp"

//endregion

//region defines

#define LOCK_BIT 0

//endregion

namespace stm {

/**
 * Lock status struct.
 */
struct lock_status_t {
    bool is_locked;
    uint64_t timestamp;
};

/**
 * Versioned Lock struct. Guards concurrent accesses to a shared variable. Uses spinlock.
 */
struct lock_t {

    std::atomic<uint64_t> l;                // saves the current timestamp and spinlock status (at position LOCK_BIT)

    /**
     * Default constructor.
     */
    lock_t() : l() {}

    /**
     * Copy constructor.
     */
    lock_t(const lock_t &other) {
        // todo seq_cst memory ordering, but some other could potentially be used
        l.store(other.l);
    }

    /**
     * Tries to acquire the lock. Fails if the lock is already acquired by some other caller.
     * @return true if the lock is acquired successfully
     */
    always_inline bool acquire() {
        uint64_t expected = 0, desired = 1;
        while (!l.compare_exchange_weak(expected, desired, std::memory_order_acquire, std::memory_order_relaxed)) {
            if (IS_SET_BIT(expected, LOCK_BIT)) return false;
            desired = SET_BIT(expected, LOCK_BIT);
        }
        return true;
    }

    /**
     * Releases the lock, and changes the timestamp.
     * @param timestamp new timestamp to set
     */
    always_inline void release(uint64_t timestamp) {
        l.store(RESET_BIT(timestamp << 1, LOCK_BIT), std::memory_order_release);
    }

    /**
     * Releases the lock without changing the timestamp.
     */
    always_inline void release() {
        l.fetch_and(~BITMASK(LOCK_BIT), std::memory_order_release);
    }

    /**
     * Queries for the current status of the lock.
     * @return current lock status
     */
    always_inline lock_status_t getStatus() {
        auto val = l.load(std::memory_order_acquire);
        return {IS_SET_BIT(val, LOCK_BIT), RESET_BIT(val >> 1, 63)};
    }

};

}

#endif
