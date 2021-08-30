//
// Created by liuzikai on 9/1/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Verification/CrossChecker.h"
#include "klc3/Generation/ReportFormatter.h"

#define AS_DIVERGE_AS_POSSIBLE  1

namespace klc3 {

vector<Issue::Type> CrossChecker::compare(State *goldState, State *testState, ConstraintSet &finalConstraints) {
    vector<Issue::Type> ret;

    for (const auto &it : checkLists) {
        if (it.first != Issue::NO_ISSUE) {

            string noteStr;
            llvm::raw_string_ostream note(noteStr);
            vector<ThingToCompare> failedThings;

            for (const auto &thing : it.second) {
                switch (thing.type) {
                    case ThingToCompare::OUTPUT:
                        if (outputDiverge(goldState, testState, finalConstraints)) {
                            failedThings.emplace_back(thing);
                        }
                        break;
                    case ThingToCompare::REG:
                        if (compareRegisters(goldState, testState, finalConstraints, thing.from, thing.to,
                                             note)) {
                            failedThings.emplace_back(thing);
                        }
                        break;
                    case ThingToCompare::LAST_INST:
                        if (compareLastInst(goldState, testState, finalConstraints, note, thing.dumpGold)) {
                            failedThings.emplace_back(thing);
                        }
                        break;
                    case ThingToCompare::MEM:
                        if (compareMem(goldState, testState, finalConstraints, thing.from, thing.to)) {
                            failedThings.emplace_back(thing);
                        }
                        break;
                }
            }

            if (!failedThings.empty()) {
                if (auto issueInfo = testState->newStateIssue(it.first, nullptr)) {
                    note.flush();
                    issueInfo->note = note.str();

                    // Additional note will be generated with callback
                    auto ptr = new CallBackArg{failedThings, goldState};
                    issueInfo->descCallback = CrossChecker::generateIssueDesc;
                    issueInfo->descCallbackArg = (void *) ptr;
                    // ptr will be deleted at callback

                    goldState->triggerNewIssue = true;  // set this flag so that it won't be recycled
                }
                ret.emplace_back(it.first);
            }
        }
    }
    return ret;
}

bool CrossChecker::outputDiverge(State *goldState, State *testState, ConstraintSet &finalConstraints) {
    bool ret = false;
    if (goldState->lc3Out.size() != testState->lc3Out.size()) {
        // Incorrect answer with incorrect length
        ret = true;
    } else {
        for (unsigned i = 0; i < goldState->lc3Out.size(); ++i) {
            if (!(exprMustEq(goldState->lc3Out[i], testState->lc3Out[i], finalConstraints))) {
                // Incorrect answer with correct length
                ret = true;
#if !AS_DIVERGE_AS_POSSIBLE
                return ret;
#endif
            }
        }
    }
    return ret;
}

bool CrossChecker::compareRegisters(State *goldState, State *testState, ConstraintSet &finalConstraints,
                                    uint16_t from, uint16_t to, llvm::raw_ostream &note) {
    vector<int> differentRegs;
    for (int r = from; r <= to; r++) {
        ref<Expr> goldVal = goldState->getReg((Reg) r);
        ref<Expr> testVal = testState->getReg((Reg) r);

        if (!exprMustEq(testVal, goldVal, finalConstraints)) {
            differentRegs.emplace_back(r);
        }
    }
    if (!differentRegs.empty()) {
        for (int r : differentRegs) {
            if (r != *differentRegs.begin()) note << ", ";
            note << "R" << r;
        }
        if (differentRegs.size() == 1) note << " doesn't match the expected value.\n";
        else note << " don't match the expected values.\n";
        return true;
    } else {
        return false;
    }
}

bool CrossChecker::compareLastInst(State *goldState, State *testState, ConstraintSet &finalConstraints,
                                   llvm::raw_ostream &note, bool dumpGold) {
    const ref<InstValue> &goldInst = goldState->latestNonOSInst[0];
    const ref<InstValue> &testInst = testState->latestNonOSInst[0];

    // Compare address rather than the inst itself, since there may be two copy of entry codes
    if (goldInst.isNull() != testInst.isNull() ||
        (!goldInst.isNull() && !testInst.isNull() && goldInst->addr != testInst->addr)) {

        note << "your last executed instruction was: " << (testInst.isNull() ? "<none>" :
                                                           reportFormatter->formattedContext(testInst))
             << ". ";
        if (dumpGold) {
            note << "The expected one is: " << (goldInst.isNull() ? "<none>" :
                                                reportFormatter->formattedContext(goldInst)) << ". ";
        }
        note << "\n";
        return true;
    } else {
        return false;
    }
}

bool CrossChecker::compareMem(State *goldState, State *testState, ConstraintSet &finalConstraints,
                              uint16_t from, uint16_t to) {
    bool ret = false;
    for (unsigned long addr = from; addr <= to; ++addr) {
        if (!(exprMustEq(goldState->mem.read(addr)->e, testState->mem.read(addr)->e, finalConstraints))) {
            ret = true;
#if !AS_DIVERGE_AS_POSSIBLE
            return ret;
#endif
        }
    }
    return ret;
}

bool CrossChecker::exprMustEq(const ref<Expr> &a, const ref<Expr> &b, ConstraintSet &finalConstraints) const {
    if (a.isNull() && b.isNull()) return true;
    if (a.isNull() != b.isNull()) return false;

    ref<Expr> eqExpr = builder->Eq(a, b);

    // mayNotEq => mustNotEq seems to be faster

    bool mayNotEq;
    bool success = solver->mayBeFalse(Query(finalConstraints, eqExpr), mayNotEq);
    assert(success && "Unexpected solver failure");

    if (mayNotEq) {

        bool mustNotEq;
        success = solver->mustBeFalse(Query(finalConstraints, eqExpr), mustNotEq);
        assert(success && "Unexpected solver failure");

        if (!mustNotEq) {
            // mayNotEq && !mustNotEq => may or may not equal

            // Add constraint to generate unequal state
            ConstraintManager(finalConstraints).addConstraint(Expr::createIsZero(eqExpr));
        }

        return false;

    } else {
        // Not mayNotEq => must be equal
        return true;
    }
}

void CrossChecker::generateIssueDesc(const Issue &issue, State *testState, const Assignment &assignment,
                                     void *ptrCallBackArg, string &desc) {
    const auto arg = static_cast<CallBackArg *>(ptrCallBackArg);

    // Collect all registers to print
    vector<Reg> regToPrintDump;
    vector<Reg> regToPrintNoDump;

    for (const auto &thing : arg->failedThings) {
        switch (thing.type) {
            case ThingToCompare::OUTPUT:
                appendOutputToIssueDesc(testState, assignment, (thing.dumpGold ? arg->goldState : nullptr), desc);
                desc += "\n";
                break;
            case ThingToCompare::REG:
                if (thing.dumpGold) {
                    for (auto r = thing.from; r <= thing.to; r++) regToPrintDump.emplace_back((Reg) r);
                } else {
                    for (auto r = thing.from; r <= thing.to; r++) regToPrintNoDump.emplace_back((Reg) r);
                }
                break;
            case ThingToCompare::LAST_INST:
                // No callback is needed
                break;
            case ThingToCompare::MEM:
                appendMemToIssueDesc(testState, assignment, thing.from, thing.to,
                                     (thing.dumpGold ? arg->goldState : nullptr), desc);
                desc += "\n";
                break;
        }
    }

    if (!regToPrintDump.empty()) {
        appendRegistersToIssueDesc(testState, assignment, regToPrintDump, arg->goldState, desc);
        desc += "\n";
    }
    if (!regToPrintNoDump.empty()) {
        appendRegistersToIssueDesc(testState, assignment, regToPrintNoDump, nullptr, desc);
        desc += "\n";
    }

    delete arg;
}

void CrossChecker::appendOutputToIssueDesc(State *testState, const Assignment &assignment, const State *goldState,
                                           string &desc) {
    llvm::raw_string_ostream note(desc);

    note << "your output (length = " << testState->lc3Out.size() << ") is:\n";
    bool hasUnprintableCharacter = reportFormatter->reportOutput(note, testState, assignment);
    note << "\n";

    if (goldState != nullptr) {  // should dump gold
        note << "The expected output (length = " << goldState->lc3Out.size() << ") is:\n";
        assert(reportFormatter->reportOutput(note, goldState, assignment) == false &&
               "Gold program prints unprintable ASCII");
        note << "\n";
    }

    reportFormatter->reportOutputComment(note, hasUnprintableCharacter);

    note.flush();
}

void CrossChecker::appendRegistersToIssueDesc(State *testState, const Assignment &assignment, const vector<Reg> &regs,
                                              const State *goldState, string &desc) {
    llvm::raw_string_ostream note(desc);

    note << "\nYour registers:\n";
    reportFormatter->reportRegisters(note, testState, assignment, regs);
    note << "\n";

    if (goldState != nullptr) {  // should dump gold
        note << "The expected registers:\n";
        reportFormatter->reportRegisters(note, goldState, assignment, regs);
        note << "\n";
    }

    note.flush();
}

void CrossChecker::appendMemToIssueDesc(State *testState, const Assignment &assignment, uint16_t start, uint16_t end,
                                        const State *goldState, string &desc) {

    llvm::raw_string_ostream note(desc);

    note << "your memory from address " << toLC3Hex(start) << " to " << toLC3Hex(end) << " is:\n";
    reportFormatter->reportMemory(note, testState, assignment, start, end);
    note << "\n";

    if (goldState != nullptr) {  // should dump gold
        note << "The expected memory is:\n";
        reportFormatter->reportMemory(note, goldState, assignment, start, end);
        note << "\n";
    }

    note.flush();
}

void CrossChecker::registerThingToCompare(Issue::Type issue, ThingToCompare thingToCompare) {
    auto it = checkLists.find(issue);
    if (it == checkLists.end()) {
        it = checkLists.emplace(issue, CheckList{}).first;
    }
    it->second.emplace_back(thingToCompare);
}

}