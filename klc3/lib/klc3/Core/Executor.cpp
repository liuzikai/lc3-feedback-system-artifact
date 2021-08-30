//
// Created by liuzikai on 3/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Core/Executor.h"
#include "klc3/Generation/ReportFormatter.h"

#define LOG_BR_FORK  0
#define LOG_SYM_ADDR_FORK 0
#define DISABLE_FORK_ON_SYM_ADDR  0

namespace klc3 {

extern llvm::cl::OptionCategory KLC3ExecutionCat;

llvm::cl::opt<int> ForkOnSymAddrThreshold(
        "cache-sym-addr-threshold",
        llvm::cl::desc("Cache a memory access of a symbolic address whose "
                       "number of possible concrete values is less than this limit "
                       "under the initial constraints. "
                       "Set to 0 to disable. Set to -1 for unlimited. (default=0)"),
        llvm::cl::init(0),
        llvm::cl::cat(KLC3ExecutionCat));

Executor::Executor(ExprBuilder *builder, Solver *solver, const map<uint16_t, ref<MemValue>> &mem)
        : WithBuilder(builder),
          symAddrCacheHits("SymAddrCacheHits", "SAHits"),
          symAddrCacheMisses("SymAddrCacheMisses", "SAMiss"), solver(solver) {

    // baseMem initialized to nullptr (by ref())
    for (const auto &m: mem) {
        baseMem[m.first] = m.second;
    }

    if (ForkOnSymAddrThreshold != 0) {
        newProgWarn() << "using the symbolic address cache with threshold " << ForkOnSymAddrThreshold
                      << ", which is only effective for well designed input spaces.\n";
    }
}

State *Executor::createInitState(const vector<ref<Expr>> &constraints, uint16_t initPC, IssuePackage *issuePackage) {

    auto *s = stateAllocator.createInitState(builder, baseMem, initPC, issuePackage);

    // Load constraints into state and check compatibility
    for (const auto &c : constraints) {
        bool res;
        bool success = solver->mustBeFalse(Query(s->constraints, c), res);
        assert(success && "Unexpected solver failure");
        s->solverCount++;
        if (res) {
            newProgErr() << "Initial constraints are provably unsatisfiable! Failed to create initial state!" << "\n"
                         << "  Triggering constraint: " << c << "\n";
            progExit();
        } else {
            s->addConstraint(c);
        }
    }

    // Save a copy of initial constraints for possibleValuesCache, for example
    initConstraints = s->constraints;

    return s;
}

bool Executor::step(State *s, StateVector &result, bool returnImmediatelyIfWillFork) {
    result.clear();

    /// Phase 1. Fetch Instruction
    ref<InstValue> ir = nullptr;
    fetchInst(s, ir);
    if (s->status != State::NORMAL) {
        result.push_back(s);
        return true;
    }

    if (returnImmediatelyIfWillFork && !ir.isNull() && ir->instID() == InstValue::BR) {
        ref<Expr> brCond;
        Solver::Validity br = solveBRCond(s, ir, brCond);
        if (br == klee::Solver::Unknown) {
            return false;
        }
    }

    // Update path info
    for (unsigned i = 1; i < State::RECORD_FETCHED_INST_COUNT; i++) {
        s->latestInst[i] = s->latestInst[i - 1];
    }
    s->latestInst[0] = ir.get();
    if (!ir->belongsToOS) {
        for (unsigned i = 1; i < State::RECORD_FETCHED_INST_COUNT; i++) {
            s->latestNonOSInst[i] = s->latestNonOSInst[i - 1];
        }
        s->latestNonOSInst[0] = ir.get();
    }
    ++s->stepCount;  // include OS node

    /// Phase 2. Update IR and PC
    updateIRAndPC(s, ir);
    if (s->status != State::NORMAL) {
        result.push_back(s);
        return true;
    }

    /// Phase 3. Execute Instruction
    switch (ir->instID()) {
        case InstValue::BR:
            executeBR(s, ir, result);
            return true;  // result is already filled
        case InstValue::LDR:
            executeLDR(s, ir, result);
            return true;
        case InstValue::LDI:
            executeLDI(s, ir, result);
            return true;
        case InstValue::STR:
            executeSTR(s, ir, result);
            return true;
        case InstValue::STI:
            executeSTI(s, ir, result);
            return true;
        case InstValue::ADD:
        case InstValue::ADDi:
            executeADD(s, ir);
            break;
        case InstValue::AND:
        case InstValue::ANDi:
            executeAND(s, ir);
            break;
        case InstValue::JMP:
            executeJMP(s, ir);
            break;
        case InstValue::JSR:
            executeJSR(s, ir);
            break;
        case InstValue::JSRR:
            executeJSRR(s, ir);
            break;
        case InstValue::LD:
            executeLD(s, ir);
            break;
        case InstValue::LEA:
            executeLEA(s, ir);
            break;
        case InstValue::NOT:
            executeNOT(s, ir);
            break;
        case InstValue::ST:
            executeST(s, ir);
            break;
        case InstValue::TRAP:
            executeTRAP(s, ir);
            break;
        case InstValue::RTI:
            if (auto issueInfo = s->newStateIssue(Issue::ERR_INVALID_INST, ir)) {
                issueInfo->setNote("RTI is not supported at " + reportFormatter->formattedContext(ir) + ".\n");
            }
            assert(s->status == State::BROKEN && "Non-continuable error");
            break;
        default:
            assert(!"Instruction is not supported yet");
    }
    result.push_back(s);
    return true;
}

void Executor::fetchInst(State *s, ref<InstValue> &ir) {
    uint16_t pc = s->getPC();

    ref<MemValue> val = s->mem.read(pc);

    if (!val.isNull() && val->type == MemValue::MEM_INST) {
        ir = dyn_cast<InstValue>(val);
    } else {
        // Use last executed inst as break point
        if (val.isNull()) {
            if (auto issueInfo = s->newStateIssue(Issue::ERR_EXECUTE_UNINITIALIZED_MEMORY, s->latestNonOSInst[0])) {
                issueInfo->setNote("trying to execute addr " + toLC3Hex(pc) + ".\n");
            }
        } else if (val->type == MemValue::MEM_DATA) {
            if (auto issueInfo = s->newStateIssue(Issue::ERR_EXECUTE_DATA_AS_INST, s->latestNonOSInst[0])) {
                issueInfo->setNote("trying to execute addr " + toLC3Hex(pc) + ".\n");
            }

        }
        assert(s->status == State::BROKEN && "Non-continuable error");
    }
}

void Executor::updateIRAndPC(State *s, const ref<InstValue> &ir) {
    setReg(s, R_IR, ir->e, ir);
    setReg(s, R_PC, (s->getPC() + 1) & 0xFFFF, ir);
}

void Executor::executeSTR(State *s, const ref<InstValue> &ir, StateVector &result) {
    handleExprST(s,
                 builder->Add(getReg(s, ir->baseR(), ir), buildConstant(ir->imm6())),
                 getReg(s, ir->sr(), ir, true),  // bypass uninitialized register null value
                 ir,
                 result);
}

void Executor::executeSTI(State *s, const ref<InstValue> &ir, StateVector &result) {
    uint16_t addr = s->getPC() + ir->imm9();  // auto wrap 0xFFFF
    ref<Expr> midAddr;
    memReadData(s, addr, midAddr, ir, -1);
    if (s->status != State::NORMAL) {
        result.push_back(s);
        return;
    }
    handleExprST(s, midAddr, getReg(s, ir->sr(), ir, true), ir, result);  // by pass null
}

void Executor::handleExprST(State *s, const ref<Expr> &addr, const ref<Expr> &value, const ref<InstValue> &ir,
                            StateVector &result) {
    if (addr->getKind() == Expr::Constant) {
        memWriteData(s, castConstant(addr), value, ir);  // if value is null, still bypass it
        result.push_back(s);
    } else {

        vector<pair<uint16_t, State *>> instances;
        forkOnRange(s, addr, ir, instances);
        assert(!instances.empty() && "Immediate address evaluate to no range, which means constraints have conflicts");

        for (auto &it : instances) {
            uint16_t realizedAddr = it.first;
            State *t = it.second;
            memWriteData(t, realizedAddr, value, ir);  // if value is null, still bypass it
            result.push_back(t);
        }
    }
}


void Executor::executeLDR(State *s, const ref<InstValue> &ir, StateVector &result) {
    Reg DR = ir->dr();
    ref<Expr> midAddr = builder->Add(getReg(s, ir->baseR(), ir), buildConstant(ir->imm6()));
    handleExprLD(s, DR, midAddr, ir, result);
}

void Executor::executeLDI(State *s, const ref<InstValue> &ir, StateVector &result) {
    Reg DR = ir->dr();
    uint16_t addr = s->getPC() + ir->imm9();
    ref<Expr> midAddr;
    memReadData(s, addr, midAddr, ir, -1);
    if (s->status != State::NORMAL) {
        result.push_back(s);
        return;
    }
    handleExprLD(s, DR, midAddr, ir, result);
}

void
Executor::handleExprLD(State *s, Reg DR, const ref<Expr> &addr, const ref<InstValue> &ir, StateVector &result) {

    if (addr->getKind() == Expr::Constant) {

        ref<Expr> data;
        memReadData(s, castConstant(addr), data, ir, DR);
        if (s->status != State::NORMAL) {
            result.push_back(s);
            return;
        }
        setReg(s, DR, data, ir);
        setCC(s, DR, ir);
        result.push_back(s);
        return;

    } else {

        vector<pair<uint16_t, State *>> instances;
        forkOnRange(s, addr, ir, instances);
        assert(!instances.empty() && "Immediate address evaluate to no range, which means constraints have conflicts");

        for (auto &it : instances) {
            uint16_t realizedAddr = it.first;
            State *t = it.second;
            ref<Expr> data;
            memReadData(t, realizedAddr, data, ir, DR);
            if (t->status != State::NORMAL) {
                result.push_back(t);
                continue;
            }
            setReg(t, DR, data, ir);
            setCC(t, DR, ir);
            result.push_back(t);
        }
        return;
    }
}

/**
 * Fork states by range of given expression
 * @param s
 * @param val
 * @param instances [out]  vector of pairs of {evaluated value, state}
 */
void Executor::forkOnRange(State *s, const ref<Expr> &val, const ref<InstValue> &ir,
                           vector<pair<uint16_t, State *>> &instances) {
    vector<pair<uint16_t, ref<Expr>>> values;
    evalPossibleValuesWithCache(val, s->constraints, values, s->solverCount);

    if (values.empty()) return;  // no valid range
    if (values.size() > 1) {  // need to fork
#if LOG_SYM_ADDR_FORK
        progInfo() << "Fork " << values.size() - 1 << " states" << "  At: " << ir->sourceContext() << "\n";
#endif
    }

#if !DISABLE_FORK_ON_SYM_ADDR
    for (unsigned i = 0; i < values.size(); i++) {
        State *t = (i != values.size() - 1 ? stateAllocator.fork(s) : s);  // realize s to the last value
        // Notice that s must be the last, or changes on it will be forked into other states
        // If there is only one values, no need to add the Eq constraint since it must be true
        if (!values[i].second.isNull() && values.size() != 1) {
            t->addConstraint(values[i].second);
        }
        instances.emplace_back(values[i].first, t);
    }
#else
    bool res;
    bool success = solver->mustBeTrue(Query(s->constraints, values.back().second), res);
    assert(success && "Unexpected solver failure");
    s->solverCount++;
    if (!res) s->addConstraint(values.back().second);
    instances.emplace_back(values.back().first, s);
#endif
}


void Executor::evalPossibleValuesWithCache(const ref<Expr> &expr, const ConstraintSet &constraints,
                                           vector<pair<uint16_t, ref<Expr>>> &result, int &solverCount) {
    assert(expr->getWidth() == Expr::Int16);

    if (ForkOnSymAddrThreshold == 0) {
        evalPossibleValues(expr, constraints, result, solverCount);
        return;
    }

    auto it = possibleValuesCache.find(expr);
    if (it == possibleValuesCache.end()) {
        ++symAddrCacheMisses;

        vector<pair<uint16_t, ref<Expr>>> resultUnderInitConstraints;  // pairs of value, expr == value

        // Evaluate the possible values under initial constraints, with a upper limit on possible value count

        if (evalPossibleValues(expr, initConstraints, resultUnderInitConstraints, solverCount,
                               ForkOnSymAddrThreshold)) {
            assert(!resultUnderInitConstraints.empty() && "Expr under initial constraints evaluates to no value!");
            it = possibleValuesCache.emplace(expr, resultUnderInitConstraints).first;
        } else {
            // Place an empty vector to indicate there are too many possible values under only initial constraints
            it = possibleValuesCache.emplace(expr, vector<pair<uint16_t, ref<Expr>>>{}).first;

            static bool alreadyWarned = false;
            if (!alreadyWarned) {
                newProgWarn() << "Possible values on an Expr under initial constraints result in too many values.\n";
                alreadyWarned = true;
            }
        }
    } else {
        ++symAddrCacheHits;
    }

    // Evaluate possible values on current constraints
    if (it->second.empty()) {

        /*
         * Evaluation on initial constraints results in too many possible values, possibly due to poorly designed input
         * space. Hope the currently evaluated program is well written, so that reasonable constraints are placed on
         * the variables and the evaluation on complete current constraints won't result in too many values. Otherwise,
         * it will get explode anyway...
         */

        evalPossibleValues(expr, constraints, result, solverCount);
    } else {

        for (const auto &item : it->second) {
            bool res, success;
            success = solver->mayBeTrue(Query(constraints, item.second), res);
            assert(success && "Unexpected solver failure");
            solverCount++;
            if (res) {
                result.emplace_back(item);
            }
        }

    }

}

bool Executor::evalPossibleValues(const ref<Expr> &expr, const ConstraintSet &constraints,
                                  vector<pair<uint16_t, ref<Expr>>> &result, int &solverCount,
                                  int maxPossibleValueCount) const {
    assert(expr->getWidth() == Expr::Int16);

    uint16_t minVal, maxVal;
    bool res, success;

    // Binary search for # of useful bits
    uint64_t lo = 0, hi = 16, mid, bits = 0;
    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        success = solver->mustBeTrue(Query(constraints,
                                           builder->Eq(builder->LShr(expr, buildConstant(mid)),
                                                       buildConstant(0))),
                                     res);
        assert(success && "Unexpected solver failure");
        solverCount++;

        if (res) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    bits = lo;

    // Binary search for min
    lo = 0, hi = klee::bits64::maxValueOfNBits(bits);
    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        success = solver->mayBeTrue(Query(constraints, builder->Ule(expr, buildConstant(mid))),
                                    res);
        assert(success && "Unexpected solver failure");
        solverCount++;

        if (res) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    minVal = lo;

    // Check for the common case that maxVal == minVal
    ref<Expr> c = builder->Eq(expr, buildConstant(minVal));
    success = solver->mustBeTrue(Query(constraints, c), res);
    assert(success && "Unexpected solver failure");
    solverCount++;
    if (res) {
        result.emplace_back(minVal, c);
        return (maxPossibleValueCount == -1 || (int) result.size() <= maxPossibleValueCount);
    }

    // Binary search for max
    lo = minVal, hi = klee::bits64::maxValueOfNBits(bits);
    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        success = solver->mustBeTrue(Query(constraints, builder->Ule(expr, buildConstant(mid))),
                                     res);
        solverCount++;
        assert(success && "FIXME: Unhandled solver failure");

        if (res) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    maxVal = lo;

    /*maxVal = klee::bits64::maxValueOfNBits(bits);*/

    return evalPossibleValuesInRange(expr, constraints, minVal, maxVal, result, solverCount, maxPossibleValueCount);
}

bool Executor::evalPossibleValuesInRange(const ref<Expr> &expr, const ConstraintSet &constraints,
                                         long minVal, long maxVal,
                                         vector<pair<uint16_t, ref<Expr>>> &result, int &solverCount,
                                         int maxPossibleValueCount) const {

    // NOTE: as we do arithmetic, minVal, maxVal and midVal below must be larger than uint16_t (and better to be signed)
    bool res, success;
    ref<Expr> c;

    if (minVal > maxVal) {
        return true;

    } else if (maxVal - minVal <= 3) {

        /**
         * [0, 3] for example
         * Direct Eq: 4 queries
         * Fall back to else below, worse case: 6 queries
         *   mid = 1
         *   1) 0 <= expr <= 1? [0, 1]
         *     2) expr == 0?
         *     3) expr == 1?
         *   4) 2 <= expr <= 3? [2, 3]
         *     5) expr == 2?
         *     6) expr == 3?
         * Best case: 3 (half of the range is impossible, while the other must be possible or it will exit earlier)
         *   1) 0 <= expr <= 1? No. Then 2 <= expr <= 3 must be true.
         *   2) expr == 2?
         *   3) expr == 3?
         */

        // NOTE: must be larger than uint16_t or ++ may overflow
        for (long val = minVal; val <= maxVal; val++) {
            c = builder->Eq(expr, buildConstant(val));
            success = solver->mayBeTrue(Query(constraints, c), res);
            assert(success && "Unexpected solver failure");
            solverCount++;
            if (res) result.emplace_back(val, c);
            if (maxPossibleValueCount != -1 && (int) result.size() > maxPossibleValueCount) return false;
        }

    } else {

        long midVal = (minVal + maxVal) / 2;

        // [minVal, midVal]
        if (minVal == midVal) {
            // Just query Eq
            if (!evalPossibleValuesInRange(expr, constraints, minVal, midVal, result, solverCount,
                                           maxPossibleValueCount))
                return false;
        } else {
            // Check whether this half is possible
            c = builder->And(
                    builder->Ule(buildConstant(minVal), expr),
                    builder->Ule(expr, buildConstant(midVal))
            );
            success = solver->mayBeTrue(Query(constraints, c), res);
            assert(success && "Unexpected solver failure");
            solverCount++;
            if (res) {
                if (!evalPossibleValuesInRange(expr, constraints, minVal, midVal, result, solverCount,
                                               maxPossibleValueCount))
                    return false;
            }
        }

        // [midVal + 1, maxVal]
        if (midVal + 1 == maxVal) {
            // Just query Eq
            if (!evalPossibleValuesInRange(expr, constraints, midVal + 1, maxVal, result, solverCount,
                                           maxPossibleValueCount))
                return false;
        } else {
            if (!res) {
                // The other half is not possible, then this half must be possible
                res = true;
            } else {
                // Check whether this half is possible
                c = builder->And(
                        builder->Ule(buildConstant(midVal + 1), expr),
                        builder->Ule(expr, buildConstant(maxVal))
                );
                success = solver->mayBeTrue(Query(constraints, c), res);
                assert(success && "Unexpected solver failure");
                solverCount++;
            }
            if (res) {
                if (!evalPossibleValuesInRange(expr, constraints, midVal + 1, maxVal, result, solverCount,
                                               maxPossibleValueCount))
                    return false;
            }
        }

    }

    return true;
}

void Executor::executeBR(State *s, const ref<InstValue> &ir, StateVector &result) {

    ref<Expr> brCond;
    Solver::Validity br = solveBRCond(s, ir, brCond);

    State *continueState = nullptr;
    State *branchState = nullptr;

    if (br == Solver::True) {  // always branch
        branchState = s;
    } else if (br == Solver::False) {  // always continue
        continueState = s;
    } else {
        // Fork state
        branchState = s;
        continueState = stateAllocator.fork(s);

        // Add constraints
        branchState->addConstraint(brCond);

        ref<Expr> continueCond = Expr::createIsZero(brCond);
        continueState->addConstraint(continueCond);
#if LOG_BR_FORK
        progInfo() << "Fork 1 state" << "  At: " << ir->sourceContext() << "\n";
#endif
    }

    if (branchState != nullptr) {
        // Execute branch
        setReg(branchState, R_PC, branchState->getPC() + ir->imm9(), ir);  // auto wrap 0xFFFF
    }

    if (continueState != nullptr) result.push_back(continueState);
    if (branchState != nullptr) result.push_back(branchState);
}

Solver::Validity Executor::solveBRCond(State *s, const ref<InstValue> &ir, ref<Expr> &brCond) {

    // Generate branch condition
    // Only using Eq, Slt and Sle in canonical mode
    switch (ir->cc()) {
        case InstValue::CC_NONE:
            brCond = nullptr;
            return klee::Solver::False;  // never branch
        case InstValue::CC_NZP:
            brCond = nullptr;
            return klee::Solver::True;  // always branch
        case InstValue::CC_N:
            brCond = builder->Slt(getCCExpr(s, ir), buildConstant(0));
            break;
        case InstValue::CC_Z:
            brCond = Expr::createIsZero(getCCExpr(s, ir));
            break;
        case InstValue::CC_P:
            brCond = builder->Slt(buildConstant(0), getCCExpr(s, ir));
            break;
        case InstValue::CC_NP:
            brCond = Expr::createIsZero(Expr::createIsZero(getCCExpr(s, ir)));
            break;
        case InstValue::CC_NZ:
            brCond = builder->Sle(getCCExpr(s, ir), buildConstant(0));
            break;
        case InstValue::CC_ZP:
            brCond = builder->Sle(buildConstant(0), getCCExpr(s, ir));
            break;
    }

    // Constant folding builder
    if (brCond->isTrue()) return klee::Solver::True;
    else if (brCond->isFalse()) return klee::Solver::False;

#if 0
    progInfo() << "  brCond: " << brCond << "\n" << "  Constraint:" << "\n";
    for (auto it = s->constraints.begin(); it != s->constraints.end(); it++) {
        progInfo() << "    " << *it << "\n";
    }
    progInfo() << "At: " << ir->sourceContext() << "\n";
#endif

    klee::Solver::Validity ret;
    bool success = solver->evaluate(Query(s->constraints, brCond), ret);
    assert(success && "Unhandled solver failure");
    s->solverCount++;

#if 0
    if (ret == klee::Solver::Validity::Unknown) {
        progInfo() << "  brCond: " << brCond << " => Unknown\n" << "  Constraint:" << "\n";
        for (auto it = s->constraints.begin(); it != s->constraints.end(); it++) {
            progInfo() << "    " << *it << "\n";
        }
    }
#endif

    return ret;
}

void Executor::executeST(State *s, const ref<InstValue> &ir) {
    memWriteData(s,
                 s->getPC() + ir->imm9(),  // auto wrap 0xFFFF
                 getReg(s, ir->sr(), ir, true), ir);  // bypass uninitialized register null value
}

void Executor::executeLD(State *s, const ref<InstValue> &ir) {
    Reg DR = ir->dr();
    uint16_t addr = s->getPC() + ir->imm9();
    ref<Expr> result;
    memReadData(s, addr, result, ir, DR);
    if (s->status != State::NORMAL) return;
    setReg(s, DR, result, ir);
    setCC(s, DR, ir);
}

void Executor::executeJMP(State *s, const ref<InstValue> &ir) {

    if (!ir->belongsToOS && ir->baseR() == R_R7) {
        if (!s->stackHasMessedUp && !s->colorStack.empty() && s->colorStack.size() < 2) {  // in main code
            if (auto issueInfo = s->newStateIssue(Issue::ERR_RET_IN_MAIN_CODE, ir)) {
                issueInfo->setNote("main code starts at: " + toLC3Hex(s->colorStack.top()) + ".\n");
            }
            // By default ERROR, but can allow WARNING or NONE
            s->stackHasMessedUp = true;
        } // otherwise, do nothing, execute it and let SubroutineTracker to handle it
    }

    if (s->status == State::NORMAL) {
        ref<Expr> newPC = getReg(s, ir->baseR(), ir);
        if (newPC->getKind() != Expr::Constant) {
            if (auto issueInfo = s->newStateIssue(Issue::ERR_SYMBOLIC_PC, ir)) {
                issueInfo->setNote("last instruction that set PC: " +
                                   (s->regChangeLocation[R_PC].isNull() ?
                                    "(none)" : reportFormatter->formattedContext(s->regChangeLocation[R_PC])) + ".\n");
            }
            assert(s->status == State::BROKEN && "Non-continuable error");
            return;
        }
        setReg(s, R_PC, newPC, ir);
    }
}

void Executor::executeJSRR(State *s, const ref<InstValue> &ir) {
    // Note: no special handle for JMP R7 for now
    ref<Expr> newPC = getReg(s, ir->baseR(), ir);
    if (newPC->getKind() != Expr::Constant) {
        if (auto issueInfo = s->newStateIssue(Issue::ERR_SYMBOLIC_PC, ir)) {
            issueInfo->setNote("last instruction that set PC: " +
                               (s->regChangeLocation[R_PC].isNull() ?
                                "(none)" : reportFormatter->formattedContext(s->regChangeLocation[R_PC])) + ".\n");
        }
        assert(s->status == State::BROKEN && "Non-continuable error");
        return;
    }
    setReg(s, R_R7, s->getPC(), ir);
    setReg(s, R_PC, newPC, ir);
}

void Executor::executeADD(State *s, const ref<InstValue> &ir) {
    Reg dr = ir->dr();
    ref<Expr> op1 = getReg(s, ir->sr1(), ir);
    ref<Expr> op2 = (ir->instDef().format == InstValue::FMT_RRR ?
                     getReg(s, ir->sr2(), ir) :
                     static_cast<ref<Expr>>(buildConstant(ir->imm5())));
    ref<Expr> result;

    // If we encounter Mul expression, it must have been constructed here, so we can safely infer its structure.
    // But the expression can be re-written by KLEE so left and right can swap. Use the convention:
    //  Left: constant. Right: Expr.

    auto mul1 = op1->getKind() == klee::Expr::Mul ? dyn_cast<klee::BinaryExpr>(op1) : nullptr;
    auto mul2 = op2->getKind() == klee::Expr::Mul ? dyn_cast<klee::BinaryExpr>(op2) : nullptr;

    if (mul1) assert(mul1->left->getKind() == klee::Expr::Constant && "Unexpected mul1 structure");
    if (mul2) assert(mul2->left->getKind() == klee::Expr::Constant && "Unexpected mul2 structure");

    if (op1 == op2) {

        // Replace adding itself by multiplying by 2
        // Left shift by 1 also works, but multiplying can handle the more general cases in else if below

        if (mul1) {

            // Fold * 2 into the Mul expression

            int mulVal = (castConstant(mul1->left) * 2) % 65536;  // any number times 65536 overflows
            if (mulVal == 0) result = buildConstant(0);
            else result = builder->Mul(buildConstant(mulVal), mul1->right);

        } else {

            result = builder->Mul(buildConstant(2), op1);

        }

    } else if ((mul1 && mul1->right == op2) || (mul2 && mul2->right == op1)) {

        // Fold OP2 into OP1 or vice versa

        auto op = (mul1 && mul1->right == op2) ? mul1 : mul2;  // the Mul expression

        int mulVal = (castConstant(op->left) + 1) % 65536;  // any number times 65536 overflows
        if (mulVal == 0) result = buildConstant(0);
        else result = builder->Mul(buildConstant(mulVal), op->right);

    } else if (mul1 && mul2 && mul1->right == mul2->right) {

        // Combine OP1 and OP2

        int mulVal = (castConstant(mul1->left) + castConstant(mul2->left)) % 65536;
        if (mulVal == 0) result = buildConstant(0);
        else result = builder->Mul(buildConstant(mulVal), mul1->right);

    } else {
        result = builder->Add(op1, op2);  // note: not masking 0xFFFF for now
    }
    setReg(s, dr, result, ir);
    setCC(s, dr, ir);
}

void Executor::executeAND(State *s, const ref<InstValue> &ir) {
    Reg dr = ir->dr();

    // Here we don't use getReg(), since we don't want to give warning when AND an uninitialized reg with 0
    ref<Expr> op1 = s->getReg(ir->sr1());
    ref<Expr> op2 = (ir->instDef().format == InstValue::FMT_RRR ?
                     s->getReg(ir->sr2()) :
                     static_cast<ref<Expr>>(buildConstant(ir->imm5())));
    if (op1.isNull()) {
        // NOTE: without optimization like constant folding, it's still possible that op2 must be zero under some
        //       constraints, but this case is ignored since it is not obvious and should be avoided
        if (op2.isNull() || !op2->isZero()) {
            if (!ir->belongsToOS) {
                if (auto issueInfo = s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_REGISTER, ir)) {
                    issueInfo->setNote("using uninitialized R" + llvm::Twine((int) ir->sr1()) + ".\n");
                }
                setReg(s, ir->sr1(), 0, ir);  // write back to warning only once
            }
        }
    }
    if (op2.isNull()) {
        if (op1.isNull() || !op1->isZero()) {
            if (!ir->belongsToOS) {
                if (auto issueInfo = s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_REGISTER, ir)) {
                    issueInfo->setNote("using uninitialized R" + llvm::Twine((int) ir->sr2()) + ".\n");
                }
                setReg(s, ir->sr2(), 0, ir);  // write back to warning only once
            }
        }
    }

    if (op1.isNull()) op1 = buildConstant(0);
    if (op2.isNull()) op2 = buildConstant(0);

    ref<Expr> result = builder->And(op1, op2);
    setReg(s, dr, result, ir);
    setCC(s, dr, ir);
}

void Executor::executeNOT(State *s, const ref<InstValue> &ir) {
    Reg DR = ir->dr();
    setReg(s, DR, builder->Not(getReg(s, ir->sr1(), ir)), ir);
    setCC(s, DR, ir);
}

void Executor::executeLEA(State *s, const ref<InstValue> &ir) {
    Reg DR = ir->dr();
    setReg(s, DR, s->getPC() + ir->imm9(), ir);  // note: not masking 0xFFFF for now
    setCC(s, DR, ir);
}

void Executor::executeJSR(State *s, const ref<InstValue> &ir) {
    setReg(s, R_R7, s->getPC(), ir);
    setReg(s, R_PC, s->getPC() + ir->imm11(), ir);  // auto wrap 0xFFFF
}

void Executor::executeTRAP(State *s, const ref<InstValue> &ir) {
    if (ir->sourceContent == "INSTANT_HALT") {
        s->status = State::HALTED;
        return;
    }

    setReg(s, R_R7, s->getPC(), ir);
    ref<Expr> newPC;
    memReadData(s, ir->vec8(), newPC, ir, -1);  // the vector table given by LC3OS must be constant
    if (s->status != State::NORMAL) return;
    setReg(s, R_PC, castConstant(newPC), ir);
}

Issue::Type Executor::memReadData(State *s, uint16_t addr, ref<Expr> &result, const ref<InstValue> &ir, int dr) const {

    Issue::Type ret = Issue::NO_ISSUE;

    ref<MemValue> value = s->mem.read(addr);

    if (value.isNull()) {  // this is nullptr MemValue, which means the memory is never wrote

        if (auto issueInfo = s->newStateIssue(Issue::WARN_POSSIBLE_WILD_READ, ir)) {
            issueInfo->setNote("reading addr " + toLC3Hex(addr) + ".\n");
            ret = Issue::WARN_POSSIBLE_WILD_READ;
        }
        // State status is set by newStateIssue
        result = buildConstant(0);  // upper level no longer need to handle null value

    } else {

        if (value->type == MemValue::MEM_DATA && dyn_cast<DataValue>(value)->forWrite) {
            if (auto issueInfo = s->newStateIssue(Issue::WARN_READ_UNINITIALIZED_MEMORY, ir)) {
                issueInfo->setNote("reading addr " + toLC3Hex(addr) + ".\n");
                ret = Issue::WARN_READ_UNINITIALIZED_MEMORY;
            }
            // State status is set by newStateIssue
        } else if (value->type == MemValue::MEM_INST) {
            // InstValue *inst = dyn_cast<InstValue>(value);
            if (auto issueInfo = s->newStateIssue(Issue::WARN_READ_INST_AS_DATA, ir)) {
                issueInfo->setNote("reading addr " + toLC3Hex(addr) + ".\n");
                ret = Issue::WARN_READ_INST_AS_DATA;
            }
            // State status is set by newStateIssue
        }

        if (value->e.isNull()) {  // this is the nullptr value, while the MemValue is not nullptr

            if (s->memStoringUninitReg.find(addr) != s->memStoringUninitReg.end()) {

                if (dr != -1 &&
                    dr == s->memStoringUninitReg[addr].first) {  // loading data into original register
                    result = nullptr;  // still store nullptr back
                } else {
                    if (auto issueInfo = s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_REGISTER, ir)) {
                        issueInfo->setNote(
                                "Using value of: R" + llvm::Twine(s->memStoringUninitReg[addr].first) + "\n" +
                                "    Stored uninitialized reg value into memory at: " +
                                reportFormatter->formattedContext(s->memStoringUninitReg[addr].second) + ".\n");
                        ret = Issue::WARN_USE_UNINITIALIZED_REGISTER;
                    }
                    // State status is set by newStateIssue

                    s->memStoringUninitReg.erase(addr);  // one warning is enough
                    result = buildConstant(0);  // upper level no longer need to handle null value
                }

            } else { // otherwise, the warning may have been already generated
                result = buildConstant(0);  // upper level no longer need to handle null value
            }

        } else {
            result = value->e;
        }

    }

    return ret;
}

Issue::Type Executor::memWriteData(State *s, uint16_t addr, const ref<Expr> &value, const ref<InstValue> &ir) {

    Issue::Type ret = Issue::NO_ISSUE;

    ref<MemValue> oldVal = s->mem.read(addr);
    if (oldVal.isNull()) {

        if (auto issueInfo = s->newStateIssue(Issue::WARN_POSSIBLE_WILD_WRITE, ir)) {
            issueInfo->setNote("writing to unspecified addr " + toLC3Hex(addr) + ".\n");
            ret = Issue::WARN_POSSIBLE_WILD_WRITE;
        }
        // State status is set by newStateIssue

    } else {

        if (oldVal->type == MemValue::MEM_DATA && dyn_cast<DataValue>(oldVal)->forRead) {

            if (auto issueInfo = s->newStateIssue(Issue::WARN_WRITE_READ_ONLY_DATA, ir)) {
                issueInfo->setNote("overwriting read-only data at addr " + toLC3Hex(addr) + ".\n");
                ret = Issue::WARN_WRITE_READ_ONLY_DATA;
            }
            // State status is set by newStateIssue

        } else if (oldVal->type == MemValue::MEM_INST) {

            // We don't allow overwriting inst since it makes changes to the flow graph
            ref<InstValue> oldInst = dyn_cast<InstValue>(oldVal);
            if (auto issueInfo = s->newStateIssue(Issue::ERR_OVERWRITE_INST, ir)) {
                issueInfo->setNote("writing to addr " + toLC3Hex(addr) + ", original: " +
                                   reportFormatter->formattedContext(oldInst) + ".\n");
            }
            assert(s->status == State::BROKEN && "Non-continuable error");
            ret = Issue::ERR_OVERWRITE_INST;
            return ret;
        }
    }

    /**
     * @note About WARN_READ_UNINITIALIZED_REGISTER at ST/STI/STR
     * ST/STI/STR can be used to store registers. In that case we don't want to give warning immediately. And if the
     * value is simply loaded back to the original register, we would consider these as the storing-loading registers
     * and give no warning. But if the value stored into memory now is loaded into another register or used by other
     * instructions, we will report WARN_READ_UNINITIALIZED_REGISTER at that time.
     *
     * This function memWriteData() is called only by ST/STI/STR, and null value of register will get passed through to
     * here.
     */

    if (value.isNull()) {  // is storing uninitialized register into memory
        s->memStoringUninitReg[addr] = {ir->sr(), ir};  // All ST/STR/STI are using ir->sr() field
        // Continue to write nullptr into memory
    } else {
        if (s->memStoringUninitReg.find(addr) != s->memStoringUninitReg.end()) {
            s->memStoringUninitReg.erase(addr);  // no longer storing an uninitialized value
        }
    }

    MemoryManager::WriteResult result = s->mem.writeData(addr, value);
    switch (result) {
        case MemoryManager::WRITE_SUCCESS:
            return ret;
        case MemoryManager::WRITE_HALT:
            s->status = State::HALTED;
            if (s->latestNonOSInst[0]->instID() != InstValue::TRAP) {
                if (auto issueInfo = s->newStateIssue(Issue::WARN_MANUAL_HALT, s->latestNonOSInst[0])) {
                    issueInfo->setNote("last executed instruction: " +
                                       reportFormatter->formattedContext(s->latestNonOSInst[0]) + ".\n");
                }
            }
            return ret;
        case MemoryManager::WRITE_DDR:
            if (value.isNull()) {
                if (auto issueInfo = s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_REGISTER, ir)) {
                    // Here I just don't border to report which register
                    ret = Issue::WARN_USE_UNINITIALIZED_REGISTER;
                }
                s->lc3Out.emplace_back(buildConstant(0));
            } else {
                s->lc3Out.push_back(value);
            }
            return ret;
        default:
            assert(!"Unknown return value from mem->writeData()");
    }
}

ref<Expr> Executor::getReg(State *s, Reg r, const ref<InstValue> &ir, bool bypassUninitialized) {
    ref<Expr> ret = s->getReg(r);
    if (ret.isNull()) {
        if (!bypassUninitialized) {
            if (!ir->belongsToOS) {
                if (auto issueInfo = s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_REGISTER, ir)) {
                    issueInfo->setNote("using uninitialized R" + llvm::Twine((int) r) + ".\n");
                }
            }
            s->setReg(r, 0);  // write back to warn only once, but do not change regChangeLocation
            ret = buildConstant(0);
        }  // Otherwise, bypass the null value to upper level
    }
    return ret;
}

void Executor::setReg(State *s, Reg r, const ref<Expr> &value, const ref<InstValue> &ir) {
    switch (r) {
        case R_PC:
            assert(value->getKind() == klee::Expr::Constant);
            s->setPC(castConstant(value));
        case R_IR:
            s->setIR(value);
            break;
        default:
            s->setReg(r, value);
            break;
    }
    s->regChangeLocation[r] = ir;
}

void Executor::setReg(State *s, Reg r, uint16_t value, const ref<InstValue> &ir) {
    switch (r) {
        case R_PC:
            s->setPC(value);
            break;
        case R_IR:
            s->setIR(buildConstant(value));
            break;
        default:
            s->setReg(r, value);
            break;
    }
    s->regChangeLocation[r] = ir;
}

void Executor::setCC(State *s, Reg r, const ref<InstValue> &ir) {
    s->setCC(r);
    s->ccChangeLocation = ir;
}

ref<Expr> Executor::getCCExpr(State *s, const ref<InstValue> &ir) {
    ref<Expr> ret = s->getCCExpr();
    if (ret.isNull()) {
        Reg r = s->getCCSrcReg();
        if (r == NUM_REGS) {
            s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_CC, ir);
        } else {
            if (auto issueInfo = s->newStateIssue(Issue::WARN_USE_UNINITIALIZED_REGISTER, ir)) {
                issueInfo->setNote("using uninitialized R" + llvm::Twine((int) r) + " (through CC code).\n");
            }
            // Write back to warn only once, but do not change regChangeLocation and ccChangeLocation
            s->setReg(r, 0);
            s->setCC(r);
        }
        ret = buildConstant(0);
    }
    return ret;
}

}