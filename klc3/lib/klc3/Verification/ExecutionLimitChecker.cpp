//
// Created by liuzikai on 12/26/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Verification/ExecutionLimitChecker.h"
#include "klc3/Generation/ReportFormatter.h"
#include "klc3/Generation/VariableInductor.h"
#include "llvm/Support/CommandLine.h"

namespace klc3 {

llvm::cl::OptionCategory KLC3ExecutionCat(
        "KLC3 execution options",
        "These options impact how klc3 tests the input program.");

llvm::cl::opt<int> SingleStateMaxStepCount(
        "max-lc3-step-count",
        llvm::cl::desc("Terminate a state when its step count reaches ... (0 for no constraint, default)"),
        llvm::cl::init(0),
        llvm::cl::cat(KLC3ExecutionCat));

llvm::cl::opt<int> SingleStateMaxOutputLength(
        "max-lc3-out-length",
        llvm::cl::desc( "Terminate a state when its output length reaches ... (0 for no constraint, default)"),
        llvm::cl::init(0),
        llvm::cl::cat(KLC3ExecutionCat));

llvm::cl::opt<int> SingleStateMaxQueryCount(
        "max-lc3-query-count",
        llvm::cl::desc( "Terminate a state when the times it queries solver reaches ... (0 for no constraint, default)"),
        llvm::cl::init(0),
        llvm::cl::cat(KLC3ExecutionCat));


void ExecutionLimitChecker::checkLimitOnState(State *s) {

    if (SingleStateMaxStepCount != 0 && s->stepCount > SingleStateMaxStepCount) {
        // The output will be appended by callback
        if (auto issueInfo = s->newStateIssue(Issue::ERR_STATE_REACH_STEP_LIMIT, nullptr)) {
            issueInfo->setNote("program doesn't halt after " + llvm::Twine(s->stepCount) + " steps.\n");
            issueInfo->descCallback = ExecutionLimitChecker::appendOutputToIssueDesc;
        }
        assert(s->status == State::BROKEN && "Non-continuable error");
        return;
    }

    if (SingleStateMaxOutputLength != 0 && (int) s->lc3Out.size() > SingleStateMaxOutputLength) {
        // The output will be appended by callback
        if (auto issueInfo = s->newStateIssue(Issue::ERR_STATE_REACH_OUTPUT_LIMIT, nullptr)) {
            issueInfo->setNote("program doesn't halt after printing " + llvm::Twine(s->lc3Out.size()) + " characters.\n");
            issueInfo->descCallback = ExecutionLimitChecker::appendOutputToIssueDesc;
        }
        assert(s->status == State::BROKEN && "Non-continuable error");
        return;
    }

    if (SingleStateMaxQueryCount != 0 && s->solverCount > SingleStateMaxQueryCount) {
        // The output will be appended by callback
        if (auto issueInfo = s->newStateIssue(Issue::ERR_STATE_REACH_QUERY_LIMIT, nullptr)) {
            issueInfo->setNote("program doesn't halt after a long time.\n");
            issueInfo->descCallback = ExecutionLimitChecker::appendOutputToIssueDesc;
        }
        assert(s->status == State::BROKEN && "Non-continuable error");
        return;
    }

}

void ExecutionLimitChecker::appendOutputToIssueDesc(const Issue &issue, State *s, const Assignment &assignment,
                                                    void *arg, string &desc) {

    assert(issue.type == Issue::ERR_STATE_REACH_STEP_LIMIT ||
           issue.type == Issue::ERR_STATE_REACH_OUTPUT_LIMIT ||
           issue.type == Issue::ERR_STATE_REACH_QUERY_LIMIT);

    (void) arg;

    llvm::raw_string_ostream note(desc);
    reportFormatter->writeDetailsOpening(note, "The output until your program is killed");
    bool hasUnprintableCharacter = reportFormatter->reportOutput(note, s, assignment);
    note << "\n";
    reportFormatter->reportOutputComment(note, hasUnprintableCharacter);
    reportFormatter->writeDetailsClosing(note);

    note.flush();
}

}