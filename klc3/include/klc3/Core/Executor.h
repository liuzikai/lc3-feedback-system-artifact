//
// Created by liuzikai on 3/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_EXECUTOR_H
#define KLC3_EXECUTOR_H

#include <klee/Statistics/Statistic.h>
#include "MemoryValue.h"
#include "State.h"
#include "MemoryValue.h"
#include "MemoryManager.h"
#include "StateAllocator.hpp"

namespace klc3 {

/**
 * @note One Executor should be used for only one program.
 * @note Life cycles of States are managed by the StateAllocator in this Executor.
 * @note Each State has an instance of MemoryManager. baseMem is managed by this Executor, which holds the memory when
 *       the program starts.
 * @note Life cycles of Arrays are not managed by this Executor.
 */
class Executor : protected WithBuilder {

public:

    Executor(ExprBuilder *builder, Solver *solver, const map<uint16_t, ref<MemValue>> &baseMem);

    State *createInitState(const vector<ref<Expr>> &constraints, uint16_t initPC, IssuePackage *issuePackage);

    bool step(State *s, StateVector &result, bool returnImmediatelyIfWillFork = false);

    State *forkState(State *s) { return stateAllocator.fork(s); }

    int getAllocatedStateCount() const { return stateAllocator.getAllocatedStateCount(); }

    int getAliveStateCount() const { return stateAllocator.getAliveStateCount(); }

    void releaseState(State *state) { stateAllocator.releaseState(state); }

    klee::Statistic symAddrCacheHits;
    klee::Statistic symAddrCacheMisses;

private:

    Solver *solver;

    ref<MemValue> baseMem[0x10000];

    ConstraintSet initConstraints;

    StateAllocator stateAllocator;

    void fetchInst(State *s, ref<InstValue> &ir);

    void updateIRAndPC(State *s, const ref<klc3::InstValue> &ir);

    void executeBR(State *s, const ref<InstValue> &ir, StateVector &result);

    Solver::Validity solveBRCond(State *s, const ref<InstValue> &ir, ref<Expr> &brCond);


    /**
     * Evaluate all possible concrete values of an expression
     * @param expr
     * @param constraints
     * @param result  vector of {value, constraint to add (null if mustBeTrue)}
     * @param solverCount
     * @param maxPossibleValueCount
     * @return
     */
    bool evalPossibleValues(const ref<Expr> &expr, const ConstraintSet &constraints,
                            vector<pair<uint16_t, ref<Expr>>> &result, int &solverCount,
                            int maxPossibleValueCount = -1) const;

    bool evalPossibleValuesInRange(const ref<Expr> &expr, const ConstraintSet &constraints,
                                   long minVal, long maxVal,
                                   vector<pair<uint16_t, ref<Expr>>> &result, int &solverCount,
                                   int maxPossibleValueCount = -1) const;

    void evalPossibleValuesWithCache(const ref<Expr> &expr, const ConstraintSet &constraints,
                                     vector<pair<uint16_t, ref<Expr>>> &result, int &solverCount);

    // Hold possible values of expression under initial constraints
    klee::ExprHashMap<vector<pair<uint16_t, ref<Expr>>>> possibleValuesCache;

    void executeADD(State *s, const ref<InstValue> &ir);

    void executeAND(State *s, const ref<InstValue> &ir);

    void executeNOT(State *s, const ref<InstValue> &ir);

    void executeJMP(State *s, const ref<InstValue> &ir);

    void executeJSR(State *s, const ref<InstValue> &ir);

    void executeJSRR(State *s, const ref<InstValue> &ir);

    void executeTRAP(State *s, const ref<InstValue> &ir);

    void executeLEA(State *s, const ref<InstValue> &ir);

    void forkOnRange(State *s, const ref<Expr> &val, const ref<InstValue> &ir,
                     vector<pair<uint16_t, State *>> &instances);

    void executeLD(State *s, const ref<InstValue> &ir);

    void executeST(State *s, const ref<InstValue> &ir);

    void executeLDI(State *s, const ref<InstValue> &ir, StateVector &result);

    void executeLDR(State *s, const ref<InstValue> &ir, StateVector &result);

    void handleExprLD(State *s, Reg DR, const ref<Expr> &addr, const ref<InstValue> &ir, StateVector &result);

    void executeSTI(State *s, const ref<InstValue> &ir, StateVector &result);

    void executeSTR(State *s, const ref<InstValue> &ir, StateVector &result);

    void handleExprST(State *s, const ref<Expr> &addr, const ref<Expr> &value, const ref<InstValue> &ir,
                      StateVector &result);

    Issue::Type memWriteData(State *s, uint16_t addr, const ref<Expr> &value, const ref<InstValue> &ir);

    /**
     * Read a memory location.
     * @param s
     * @param addr
     * @param result  Output result. If trying to read an value that comes from an uninitialized register and storing
     *                it back to the exactly the same one (recognized as the behavior of storing-loading registers,
     *                see note in memWriteData()), the value may be null to keep the register uninitialized. In other
     *                case, uninitialized memory/register value will be replaced by 0 after giving warning.
     * @param ir
     * @param dr      If the data is to be read into a register, pass the register to here. If not pass -1.
     * @return
     */
    Issue::Type memReadData(State *s, uint16_t addr, ref<Expr> &result, const ref<InstValue> &ir, int dr) const;

    ref<Expr> getReg(State *s, Reg reg, const ref<InstValue> &ir, bool bypassUninitialized = false);

    void setReg(State *s, Reg r, const ref<Expr> &value, const ref<InstValue> &ir);

    void setReg(State *s, Reg r, uint16_t value, const ref<InstValue> &ir);

    void setCC(State *s, Reg r, const ref<InstValue> &ir);

    ref<Expr> getCCExpr(State *s, const ref<InstValue> &ir);
};

}

#endif //KLEE_EXECUTOR_H
