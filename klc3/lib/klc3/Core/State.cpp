//
// Created by liuzikai on 3/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Core/State.h"
#include "klc3/FlowAnalysis/FlowGraph.h"

namespace klc3 {

State::State(ExprBuilder *builder, ref<MemValue> *baseMemArray, uint16_t PC, IssuePackage *issuePackage)
        : WithBuilder(builder), mem(builder, baseMemArray), issuePackage(issuePackage), constraintManager(constraints),
          /* reg[] nullptr by default, uninitialized */ pc(PC), ir(buildConstant(0)),
          ccRef(NUM_REGS) /* uninitialized CC */ {}

State::State(const State &s)
        : WithBuilder(s.builder), status(s.status), constraints(s.constraints),
          mem(s.mem), /* should copy memory instead of link */
          issuePackage(s.issuePackage),
          lc3Out(s.lc3Out),
          latestInst(s.latestInst), latestNonOSInst(s.latestNonOSInst),
          stepCount(s.stepCount), solverCount(s.solverCount),
          regChangeLocation(s.regChangeLocation), ccChangeLocation(s.ccChangeLocation),
          memStoringUninitReg(s.memStoringUninitReg), statePath(s.statePath),
          colorStack(s.colorStack), jsrStack(s.jsrStack), stackHasMessedUp(s.stackHasMessedUp),
          loopStack(s.loopStack), constraintManager(constraints),
          /* --- private members --- */
          pc(s.pc), ir(s.ir), reg(s.reg), ccRef(s.ccRef) {

    // Should not copy coveredNewEdge, coveredNewSegment and avoidLoopReductionPostpone

    assert(status == NORMAL && "Only normal state should be forked");
}

State *State::fork() const {
    auto *s = new State(*this);
    assert(constraints.size() == s->constraints.size() && "Lost constraints during forking!");
    return s;
}

void State::dumpRegs(llvm::raw_ostream &os) const {
    // Dump R0 - R7
    for (int i = R_R0; i <= R_R7; i++) {
        os << "R" << i << ": ";
        if (reg[(Reg) i].isNull()) {
            os << "{uninitialized}    ";
        } else if (reg[(Reg) i]->getKind() == Expr::Constant) {
            os << toLC3Hex(castConstant(reg[(Reg) i])) << "    ";
        } else {
            reg[(Reg) i]->print(os);
            os << "\n";
        }
    }
    os << "\n";

    // Dump PC
    os << "PC: " << toLC3Hex(getPC());
    ref<MemValue> val = mem.read(getPC());
    if (!val.isNull()) {
        os << "   " << val->sourceContext() << "\n";
    } else {
        os << "   " << "(uninitialized addr)" << "\n";
    }

}

IssuePackage::IssueInfo *State::newStateIssue(Issue::Type type, const ref<InstValue> &location) {
    ref<InstValue> loc = location;
    if (!location.isNull()) {  // allow null location
        if (latestNonOSInst[0].get() != latestInst[0].get()) {
            loc = latestNonOSInst[0];
        }
    }
    IssuePackage::IssueInfo *ret = issuePackage->raiseIssue(type, loc);
    if (ret) {
        ret->s = this;
        ret->stepCount = stepCount;
        // Other fields remain to be filled by the caller

        triggerNewIssue = true;  // only set this flag if the issue is accepted
    }
    // Regardless of whether the issue is accepted
    if (issuePackage->getIssueLevel(type) == Issue::ERROR) {
        status = BROKEN;
    }
    return ret;
}

void State::dumpPath() const {
    for (const auto &edge : statePath) {
        edge->from()->dump();
        progErrs() << " -> ";
        edge->to()->dump();
        progErrs() << " ...\n";
    }
}


}