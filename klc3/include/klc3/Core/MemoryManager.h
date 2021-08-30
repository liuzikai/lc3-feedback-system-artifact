//
// Created by liuzikai on 6/26/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLEE_MEMORYMANAGER_H
#define KLEE_MEMORYMANAGER_H

#include "MemoryValue.h"


namespace klc3 {

/**
 * Memory manager
 */
class MemoryManager : protected WithBuilder {
public:

    // Constructor for baseMem and init state mem
    MemoryManager(ExprBuilder *builder, ref<MemValue> *baseMemArray);

    // Copy constructor
    MemoryManager(const MemoryManager &m) = default;

    /**
     * Read memory for a constant address
     * @param addr
     * @return MemValue at the given address. If the memory location is not unspecified, return nullptr.
     *         (ref<MemValue>.isNull(), rather than e == nullptr. See MemValue.)
     */
    ref<MemValue> read(uint16_t addr) const;

    enum WriteResult {
        WRITE_SUCCESS,
        WRITE_HALT,
        WRITE_DDR
    };
    WriteResult write(uint16_t addr, const ref<MemValue> &value);

    // Helper functions (also work on constant addresses)
    WriteResult writeData(uint16_t addr, const ref<Expr> &value);

    WriteResult writeInst(uint16_t addr, uint16_t IR);

    size_t memUsage() const { return mem.size(); }

private:

    ref<MemValue> *baseMem;

    std::unordered_map<uint16_t, ref<MemValue>> mem;  // memory slots
};

}


#endif //KLEE_MEMORYMANAGER_H
