//
// Created by liuzikai on 6/26/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Core/MemoryManager.h"

namespace klc3 {

MemoryManager::MemoryManager(ExprBuilder *builder, ref<MemValue> *baseMemArray)
        : WithBuilder(builder), baseMem(baseMemArray) {
}

ref<MemValue> MemoryManager::read(uint16_t addr) const {
    switch (addr) {
        // TODO: (maybe) add support for keyboard input
        case 0xFE00:  /* KBSR */
            // Always return the status of no available key
            return DataValue::alloc(0xFE00, buildConstant(0));
        case 0xFE02:  /* KBDR */
            break;  // no support for keyboard input yet, keep going and return memory at addr 0xFE02
        case 0xFE04:  /* DSR */
            // No rand device yet, always return ready status
            return DataValue::alloc(0xFE04, buildConstant(0x8000));
        case 0xFE06:  /* DDR */
            return DataValue::alloc(0xFE06, buildConstant(0));
        case 0xFFFE:  /* MCR */
            // No rand device yet, always return ready status
            return DataValue::alloc(0xFFFE, buildConstant(0x8000));
        default:
            break;
    }

    // Find in this level
    auto it = mem.find(addr);
    if (it != mem.end()) {
        // For symbolic DataValue, its expression is already constructed as ReadExpr
        return it->second;
    }

    // Read the base memory
    return baseMem[addr];
}

MemoryManager::WriteResult MemoryManager::write(uint16_t addr, const ref<MemValue> &value) {
    switch (addr) {
        case 0xFE00:  /* KBSR */
        case 0xFE02:  /* KBDR */
        case 0xFE04:  /* DSR */
            return WRITE_SUCCESS;
        case 0xFE06:  /* DDR */
            return WRITE_DDR;
        case 0xFFFE:  /* MCR */
            if (value->e->getKind() == Expr::Constant) {
                if ((castConstant(value->e) & 0x8000) == 0) {
                    return WRITE_HALT;
                }
            } else {
                // TODO: (maybe) handle symbolic write to MCR
                return WRITE_HALT;  // just halt...
            }
            return WRITE_SUCCESS;
        default:
            break;
    }

    mem[addr] = value;

    return WRITE_SUCCESS;
}

MemoryManager::WriteResult MemoryManager::writeData(uint16_t addr, const ref<Expr> &value) {
    ref<MemValue> v = DataValue::alloc(addr, value);
    return write(addr, v);
}

MemoryManager::WriteResult MemoryManager::writeInst(uint16_t addr, uint16_t IR) {
    ref<MemValue> v = InstValue::alloc(addr, buildConstant(IR));
    return write(addr, v);
}

}