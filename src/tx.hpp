#ifndef CS_453_2023_PROJECT_TX_HPP
#define CS_453_2023_PROJECT_TX_HPP


//region includes

#include <unordered_set>
#include <map>

#include "lock.hpp"
#include "memory.hpp"

//endregion


//region set entries

namespace stm {

struct write_set_entry_t {
    uint64_t data;
    lock_t *lock;
};

struct read_set_entry_t {
    virtual_addr_t virtual_addr;
    lock_t *lock;

    read_set_entry_t(virtual_addr_t virtual_addr, lock_t* lock) : virtual_addr(virtual_addr), lock(lock) {}

    bool operator==(const read_set_entry_t& other) const {
        return virtual_addr == other.virtual_addr;
    }

};

}

namespace std {

template<> struct hash<stm::read_set_entry_t> {
    std::size_t operator()(const stm::read_set_entry_t& read_set_entry) const noexcept {
        return read_set_entry.virtual_addr;
    }
};

}

//endregion


//region tx

namespace stm {

struct readonly_tx_t {

    uint64_t startTimestamp = -1;


    always_inline void setStartTime(clock_t &clock) {
        startTimestamp = clock.load(std::memory_order_relaxed);
    }

    inline bool read(memory_region_t& memory_region, virtual_addr_t virtual_addr, size_t size, uint64_t *dst) {
        auto end = virtual_addr + size;
        for (; virtual_addr < end; virtual_addr += ALIGNMENT, dst += 1) {

            // check whether the address is in the valid range
            auto &word = memory_region[virtual_addr];
            auto lock_status = word.lock.getStatus();

            // if the word is locked, or it has been written to later than startTimestamp, roll back
            if (lock_status.is_locked || lock_status.timestamp > startTimestamp) {
                return false;
            }

            // copy the data
            *dst = word.data;

            auto lock_status_after = word.lock.getStatus();
            // if the word is locked, or it has been written to later than startTimestamp, the obtained value
            // could be incorrect, roll back
            if (lock_status_after.is_locked || lock_status_after.timestamp > startTimestamp) {
                return false;
            }

        }

        return true;
    }

};

struct update_tx_t {

    uint64_t startTimestamp = -1;

    std::unordered_set<read_set_entry_t> read_set;                          // unordered
    std::map<virtual_addr_t, write_set_entry_t> write_set;                  // ordered (red-black tree)

    always_inline void setStartTime(clock_t &clock) {
        startTimestamp = clock.load(std::memory_order_relaxed);
    }

    always_inline void rollback() {
        read_set.clear();
        write_set.clear();
    }

    inline bool read(memory_region_t& memory_region, virtual_addr_t virtual_addr, size_t size, uint64_t* dst) {
        auto end = virtual_addr + size;
        for (; virtual_addr < end; virtual_addr += ALIGNMENT, dst += 1) {

            // if the word has already been written to, read the written value
            auto kvPair = write_set.find(virtual_addr);
            if (kvPair != write_set.end()) {
                // entry is found in the write set
                // do not add it to the read set
                *dst = kvPair->second.data;
                continue;
            }

            // check whether the address is in the valid range
            auto& word = memory_region[virtual_addr];
            auto lock_status = word.lock.getStatus();

            // if the word is locked, or it has been written to later than startTimestamp, roll back
            if (lock_status.is_locked || lock_status.timestamp > startTimestamp) {
                rollback();
                return false;
            }

            // copy the data
            *dst = word.data;

            // tod does this matter?
            auto lock_status_after = word.lock.getStatus();
            if (lock_status_after.is_locked || lock_status.timestamp != lock_status_after.timestamp) {
                // do not allow the entry to be locked, as then read set validation will fail at commit time
                rollback();
                return false;
            }

            read_set.emplace(virtual_addr, &word.lock);
        }

        return true;
    }


    inline bool write(memory_region_t& memory_region, const uint64_t* src, size_t size, virtual_addr_t virtual_addr) {
        auto end = virtual_addr + size;
        for (; virtual_addr < end; virtual_addr += ALIGNMENT, src += 1) {
            // check whether the address is in the valid range
            auto& word = memory_region[virtual_addr];
            write_set[virtual_addr] = {*src, &word.lock};
        }
        return true;
    }


    bool commit(memory_region_t& memory_region) {

        // acquire locks in the write set, in ascending order
        // remark: acquiring in ascending order is not necessary, as there's no hold&wait
        for (auto it = write_set.begin(); it != write_set.end(); it++) {
            if (!it->second.lock->acquire()) {

                if (it != write_set.begin()) {
                    do (--it)->second.lock->release();
                    while (it != write_set.begin());
                }

                rollback();
                return false;
            }
        }

        // todo thread fence, could this be reordered?
        auto commitTimestamp = memory_region.clock.fetch_add(1, std::memory_order_relaxed) + 1;

        // if commitTime == startTime + 1, no other tx wrote to the shared memory
        // otherwise, validate read set
        if (commitTimestamp != startTimestamp + 1) {

            // what if a location is first read, then written to,
            for (auto& entry: read_set) {
                auto lock_status = entry.lock->getStatus();

                if (lock_status.timestamp > startTimestamp ||
                    lock_status.is_locked && write_set.find(entry.virtual_addr) == write_set.end()) {

                    for (auto &it: write_set) it.second.lock->release();
                    rollback();
                    return false;
                }
            }

        }

        // commit changes
        for (const auto &it : write_set) {

            // find the memory word
            auto virtual_addr = it.first;
            auto& entry = it.second;
            auto& word = memory_region[virtual_addr];

            // write the data
            word.data = entry.data;

            // unlock
            entry.lock->release(commitTimestamp);
        }

        read_set.clear();
        write_set.clear();
        return true;
    }
};

}


//endregion


#endif
