//
// Created by liuzikai on 3/22/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_MEMORYVALUE_H
#define KLC3_MEMORYVALUE_H

#include "klc3/Common.h"

namespace klc3 {

// Forward declared dependency
class Node;

class Edge;

class InstValue;

class MemValue;

/**
 * A value in memory.
 *
 * If a ref<MemValue>.isNull(), it means the memory location is not specified, which is related to
 * WARN_POSSIBLE_WILD_READ/WRITE.
 *
 * If a ref<MemValue>->e.isNull(), it means the memory location holds bits (for example, by storing uninitialized
 * register value into memory), which is related to issues of uninitialized memory or register.
 */
class MemValue {
public:

    MemValue(uint16_t addr, ref <Expr> e) : addr(addr), e(std::move(e)) {}

    // Since MemValue and its subclasses are intended to be ref-managed, its addr and e must not be changed to avoid
    // interference between states. Whenever trying to write a new value into the memory, create a new instance.
    const uint16_t addr;
    const ref <Expr> e;

    enum Type {
        MEM_INST,
        MEM_DATA
    };

    Type type;

    vector <string> labels;

    string getLabel() const {  // return the first label or construct one using addr
        if (labels.empty()) {
            return toLC3Hex(addr).c_str();
        } else {
            return labels[0];
        }
    }

    string sourceFile;
    int sourceLine = -1;
    string sourceContent;

    bool belongsToOS = false;

    string sourceLocation() const {
        return sourceFile + ":" + std::to_string(sourceLine);
    }

    string sourceContext() const {
        return "[" + sourceFile + ":" + std::to_string(sourceLine) + "] " + sourceContent;
    }

    // Required by klee::ref-managed objects
    class klee::ReferenceCounter _refCount;

    static bool classof(const MemValue *V) { return true; }
};

/**
 * A memory value interpreted as an instruction
 */
class InstValue : public MemValue {
public:

    enum InstID {
        ADD,
        ADDi,
        AND,
        ANDi,
        BR,
        JMP,
        JSR,
        JSRR,
        LD,
        LDI,
        LDR,
        LEA,
        NOT,
        ST,
        STI,
        STR,
        TRAP,
        RTI,
        INST_COUNT
    };

    // Instruction fields for output formatting
    enum InstField {
        FMT_R1 = 0x001, /* DR or sr                      */
        FMT_R2 = 0x002, /* SR1 or BaseR                  */
        FMT_R3 = 0x004, /* SR2                           */
        FMT_CC = 0x008, /* condition codes               */
        FMT_IMM5 = 0x010, /* imm5                          */
        FMT_IMM6 = 0x020, /* imm6                          */
        FMT_VEC8 = 0x040, /* vec8                          */
        FMT_ASC8 = 0x080, /* 8-bit ASCII                   */
        FMT_IMM9 = 0x100, /* label (or address from imm9)  */
        FMT_IMM11 = 0x200, /* label (or address from imm11) */
        FMT_IMM16 = 0x400  /* full instruction in hex       */
    };


    // Instruction formats for output
    enum InstFormat {
        FMT_ = 0,
        FMT_RRR = (FMT_R1 | FMT_R2 | FMT_R3),
        FMT_RRI = (FMT_R1 | FMT_R2 | FMT_IMM5),
        FMT_CL = (FMT_CC | FMT_IMM9),
        FMT_R = FMT_R2,
        FMT_L = FMT_IMM11,
        FMT_RL = (FMT_R1 | FMT_IMM9),
        FMT_RRI6 = (FMT_R1 | FMT_R2 | FMT_IMM6),
        FMT_RR = (FMT_R1 | FMT_R2),
        FMT_V = FMT_VEC8,
        FMT_A = FMT_ASC8,
        FMT_16 = FMT_IMM16
    };


    // Instruction flags
    enum InstFlag {
        FLG_NONE = 0,
        FLG_SUBROUTINE = 1,
        FLG_RETURN = 2
    };

    // Instruction definitions
    struct InstDef {
        InstID id;
        const char *name;
        InstFormat format;
        uint16_t mask;
        uint16_t match;
        uint16_t flags;
    };

    static constexpr InstDef INST_DEF[] = {
            {ADD,  "ADD",  FMT_RRR,  0xF038, 0x1000, FLG_NONE},
            {ADDi, "ADD",  FMT_RRI,  0xF020, 0x1020, FLG_NONE},
            {AND,  "AND",  FMT_RRR,  0xF038, 0x5000, FLG_NONE},
            {ANDi, "AND",  FMT_RRI,  0xF020, 0x5020, FLG_NONE},
            {BR,   "BR",   FMT_CL,   0xF000, 0x0000, FLG_NONE},
            {JMP,  "JMP",  FMT_R,    0xFE3F, 0xC000, FLG_NONE},
            {JSR,  "JSR",  FMT_L,    0xF800, 0x4800, FLG_SUBROUTINE},
            {JSRR, "JSRR", FMT_R,    0xFE3F, 0x4000, FLG_SUBROUTINE},
            {LD,   "LD",   FMT_RL,   0xF000, 0x2000, FLG_NONE},
            {LDI,  "LDI",  FMT_RL,   0xF000, 0xA000, FLG_NONE},
            {LDR,  "LDR",  FMT_RRI6, 0xF000, 0x6000, FLG_NONE},
            {LEA,  "LEA",  FMT_RL,   0xF000, 0xE000, FLG_NONE},
            {NOT,  "NOT",  FMT_RR,   0xF03F, 0x903F, FLG_NONE},
            {ST,   "ST",   FMT_RL,   0xF000, 0x3000, FLG_NONE},
            {STI,  "STI",  FMT_RL,   0xF000, 0xB000, FLG_NONE},
            {STR,  "STR",  FMT_RRI6, 0xF000, 0x7000, FLG_NONE},
            {TRAP, "TRAP", FMT_V,    0xFF00, 0xF000, FLG_SUBROUTINE},
            {RTI,  "RTI",  FMT_,     0x8000, 0x8000, FLG_NONE},
    };

    // Condition codes definitions
    enum CondCode {
        CC_NONE = 0,
        CC_P = 1,
        CC_Z = 2,
        CC_ZP = 3,
        CC_N = 4,
        CC_NP = 5,
        CC_NZ = 6,
        CC_NZP = 7
    };

    InstValue(uint16_t addr, ref <Expr> IR) : MemValue(addr, std::move(IR)) {}

    /**
     * Dynamically allocate an InstValue
     * @param addr
     * @param IR
     * @return pointer of allocated InstValue or nullptr if failed to match any instruction
     * @note not supporting symbolic instruction, e must be ConstantExpr
     */
    static ref <InstValue> alloc(uint16_t addr, const ref <Expr> &IR) {
        assert(IR->getKind() == Expr::Constant && "Not supporting symbolic instruction");
        InstValue *ptr = new InstValue(addr, IR);
        if (!ptr->matchInst()) return nullptr;
        ref<MemValue> r(ptr);
        r->type = MEM_INST;
        return r;
    }

    bool isRET() const { return (instID() == JMP && baseR() == 7); }

    uint16_t ir() const { return castConstant(e); }

    Reg dr() const { return (Reg) ((ir() >> 9) & 0x7); }

    Reg sr1() const { return (Reg) ((ir() >> 6) & 0x7); }

    Reg sr2() const { return (Reg) ((ir() >> 0) & 0x7); }

    Reg baseR() const { return sr1(); }

    Reg sr() const { return dr(); }

    CondCode cc() const { return (CondCode) ((ir() & 0x0E00) >> 9); }

    uint16_t vec8() const { return ir() & 0xFF; }

    enum VEC8 {
        VEC8_GETC = 0x20,
        VEC8_OUT = 0x21,
        VEC8_PUTS = 0x22,
        VEC8_IN = 0x23,
        VEC8_PUTSP = 0x24,
        VEC8_HALT = 0x25,
        VEC8_KLC3_COMMAND = 0xC3
    };

    uint16_t imm5() const { return ((ir() & 0x010) == 0 ? (ir() & 0x00F) : (ir() | ~0x00F)); }

    uint16_t imm6() const { return ((ir() & 0x020) == 0 ? (ir() & 0x01F) : (ir() | ~0x01F)); }

    uint16_t imm9() const { return ((ir() & 0x100) == 0 ? (ir() & 0x1FF) : (ir() | ~0x1FF)); }

    uint16_t imm11() const { return ((ir() & 0x400) == 0 ? (ir() & 0x3FF) : (ir() | ~0x3FF)); }

    InstDef instDef() const {
        return def_;
    }

    InstID instID() const {
        return def_.id;
    };

    string disassemble() const;

    void dump() const { progErrs() << getLabel() << ": " << sourceContext() << "\n"; }

    Node *node = nullptr;

private:

    InstDef def_;

    bool matchInst();

public:
    static bool classof(const MemValue *V) {
        return V->type == MEM_INST;
    }

    static bool classof(const InstValue *) { return true; }
};

/**
 * A value in memory interpreted as data
 */
class DataValue : public MemValue {
public:

    DataValue(uint16_t addr, ref <Expr> e) : MemValue(addr, std::move(e)) {}

    static ref <DataValue> alloc(uint16_t addr, const ref <Expr> &e, bool forRead = false, bool forWrite = false) {
        ref<DataValue> r(new DataValue(addr, e));
        r->type = MEM_DATA;
        r->forRead = forRead;
        r->forWrite = forWrite;
        return r;
    }

    MemValue *root = nullptr;
    int offset = -1;
    bool forRead = false;
    bool forWrite = false;

    static bool classof(const MemValue *V) {
        return V->type == MEM_DATA;
    }

    static bool classof(const InstValue *) { return true; }
};

}

#endif //KLEE_MEMORYVALUE_H
