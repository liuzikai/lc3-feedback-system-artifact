//
// Created by liuzikai on 3/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_STATE_H
#define KLC3_STATE_H

#include "klc3/Common.h"
#include "klc3/Core/MemoryValue.h"
#include "klc3/Core/MemoryManager.h"
#include "klc3/Verification/IssuePackage.h"
#include "klc3/FlowAnalysis/FlowGraphBasic.h"

namespace klc3 {

class Loop;

class State : protected WithBuilder {
public:

    int getUID() const { return uid; }

    enum Status {
        NORMAL,
        HALTED,
        BROKEN,
        STATUS_COUNT
    };
    Status status = NORMAL;

    ConstraintSet constraints;

    void addConstraint(const ref<Expr> &e) { constraintManager.addConstraint(e); }

    /**
     * Get value stored in a register
     * @param r  The register index
     * @return The value stored in the given register, or nullptr if not initialized
     */
    ref<Expr> getReg(Reg r) const {
        assert(r <= R_R7 && "Use getReg only for R0-R7");
        return reg[r];
    }

    uint16_t getPC() const { return pc; }

    ref<Expr> getIR() const { return ir; }

    ref<Expr> getCCExpr() const {
        // NUM_REGS for uninitialized CC
        return ccRef <= R_R7 ? reg[ccRef] : nullptr;
    }

    Reg getCCSrcReg() const { return ccRef; }

    void setReg(Reg r, const ref<Expr> &value) {
        assert(r <= R_R7 && "Use getReg only for R0-R7");
        reg[r] = value;
    }

    void setReg(Reg r, uint16_t value) { setReg(r, buildConstant(value)); }

    void setCC(Reg r) { ccRef = r; }

    void setPC(uint16_t value) { pc = value; }

    void setIR(const ref<Expr> &value) { ir = value; }

    void dumpRegs(llvm::raw_ostream &os) const;

    MemoryManager mem;
    IssuePackage *issuePackage;

    vector<ref<Expr>> lc3Out;

    /**
     * Propose a new state issue. If non-null location is given, this function assert that it is the last node of path.
     * For global issue that doesn't associated with any state, use IssuePackage::newGlobalIssue.
     * @param type
     * @param location  Can be null. If not null, function asserts that location == latestNonOSInst[0] with the only
     *                  exception of TRAP (in which case location will be corrected to the TRAP.)
     * @return IssueInfo to fill in additional information. Nullptr if the issue is discarded.
     * @note This function sets the state to BROKEN for an ERROR level issue. Caller should assert for must-ERROR
     *       issues.
     */
    IssuePackage::IssueInfo *newStateIssue(Issue::Type type, const ref<InstValue> &location);

    // ================ Filled by IssuePackage ================
    bool triggerNewIssue = false;  // should not get copied when fork

    // ================ Filled by Executor ================
    static constexpr unsigned RECORD_FETCHED_INST_COUNT = 2;
    array<ref<InstValue>, RECORD_FETCHED_INST_COUNT> latestInst;
    array<ref<InstValue>, RECORD_FETCHED_INST_COUNT> latestNonOSInst;
    // Set when fetching instruction, [0] may or may not get executed successfully finally

    int stepCount = 0;  // include OS nodes
    int solverCount = 0;

    void dumpPath() const;

    array<ref<InstValue>, NUM_REGS> regChangeLocation;
    ref<InstValue> ccChangeLocation;
    unordered_map<uint16_t, pair<Reg, ref<InstValue>>> memStoringUninitReg;

    // ================ Filled by CoverageTracker ================
    bool coveredNewEdge = false;  // should not get copied when fork
    Path statePath;  // guiding edges (see Edge::isGuidingEdge(), init PC edge included) + last edge till HALTED/BROKEN

    // ================ Filled by SubroutineTracker ================
    stack<uint16_t> colorStack;
    stack<Node *> jsrStack;
    bool stackHasMessedUp = false;

    // ================ Filled by PruningSearcher ================

    struct LoopLayer {
        Loop *loop = nullptr;
        Path loopPathFromEntry;
        uint16_t loopColor = 0;
    };

    vector<LoopLayer> loopStack;

    bool coveredNewSegment = false;  // should not get copied when fork
    bool avoidLoopReductionPostpone = false;  // should not get copied when fork

    // NOTICE: whenever adding fields, make sure it get copied in copy constructor

private:

    /// State can only be created, forked and released at StateAllocator

    /**
     * Constructor for initial state
     * @param builder
     * @param baseMemArray
     * @param issuePackage
     * @param PC
     */
    State(ExprBuilder *builder, ref<MemValue> *baseMemArray, uint16_t PC, IssuePackage *issuePackage);

    /**
     * Copy constructor to fork
     * @param s
     * @note raw_string_ostream needs to be flushed before calling this constructor
     */
    State(const State &s);

    State *fork() const;  // can only called from StateAllocator for unified management

    int uid;  // unique ID assigned by the StateAllocator

    friend class StateAllocator;

private:

    ConstraintManager constraintManager;

    uint16_t pc;
    ref<Expr> ir;
    array<ref<Expr>, 8> reg;  // R0-R7, nullptr for uninitialized register;
    Reg ccRef; // the last Reg that was set to be CC, NUM_REGS for uninitialized CC

    // NOTICE: whenever adding fields, make sure it get copied in copy constructor
};

}

#endif //KLEE_STATE_H
