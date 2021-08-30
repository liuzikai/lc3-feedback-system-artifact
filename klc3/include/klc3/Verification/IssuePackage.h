//
// Created by liuzikai on 6/27/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_ISSUETRACKER_H
#define KLC3_ISSUETRACKER_H

#include "klc3/Common.h"
#include "Issue.h"

namespace klc3 {

// Forward declaration
class State;

class IssuePackage {
public:

    IssuePackage() = default;

    IssuePackage(const IssuePackage &issuePackage) = default;

    using IssueDescCallback = void (*)(const Issue &issue, State *s, const Assignment &assignment, void *arg,
                                       string &desc);

    struct IssueInfo {
        State *s = nullptr;
        int stepCount = -1;

        string note;
        IssueDescCallback descCallback = nullptr;
        void *descCallbackArg = nullptr;

        mutable string generatedCaseName;

        void setNote(const llvm::Twine &st) { note = st.str(); }
    };

    struct IssueStepLess {
        bool operator()(const IssueInfo &l, const IssueInfo &r) const {
            return (l.stepCount < r.stepCount);
        }
    };

    /**
     * Raise an issue without associating with a State.
     * @param type
     * @param location
     * @return IssueInfo to be filled in with additional information. Nullptr if the issue is discarded.
     */
    IssueInfo *newGlobalIssue(Issue::Type type, const ref<MemValue> &location);

    const auto &getIssues() const { return issues; }

    Issue::Type registerIssue(const string &name, Issue::Level level);

    /**
     * Get Issue Type of a user defined issue.
     * @param name
     * @return The issue type. NO_ISSUE if not found.
     */
    Issue::Type getIssueByName(const string &name) const;

    void setIssueTypeName(Issue::Type type, const string &name) { issueName[type] = name; }

    void setIssueTypeDesc(Issue::Type type, const string &desc) { issueDesc[type] = desc; }

    void setIssueTypeHint(Issue::Type type, const string &hint) { issueHint[type] = hint; }

    void setIssueTypeLevel(Issue::Type type, Issue::Level level);

    llvm::raw_ostream &writeIssueName(llvm::raw_ostream &out, const Issue::Type &issueType) const;

    llvm::raw_ostream &writeIssueDesc(llvm::raw_ostream &out, const Issue::Type &issueType) const;

    // Location is needed to generate more specific hint
    string getIssueHint(const Issue &issue) const;

    Issue::Level getIssueLevel(Issue::Type issueType) const;

    void callBackForAllDesc(const map<const State *, Assignment> &assignments);

private:

    /**
     * Raise a new issue within a State.
     * @param type
     * @param location
     * @return IssueInfo to be filled in with additional information. Nullptr if the issue is discarded.
     */
    IssueInfo *raiseIssue(Issue::Type type, const ref<MemValue> &location);

    friend class State;

    map<Issue, vector<IssueInfo>, IssueLocationLess> issues;

    unordered_map<Issue::Type, string> issueName = {
            {Issue::WARN_COMPILE,                       "WARN_COMPILE"},
            {Issue::WARN_POSSIBLE_WILD_READ,            "WARN_POSSIBLE_WILD_READ"},
            {Issue::WARN_READ_UNINITIALIZED_MEMORY,     "WARN_READ_UNINITIALIZED_MEMORY"},
            {Issue::WARN_USE_UNINITIALIZED_REGISTER,    "WARN_USE_UNINITIALIZED_REGISTER"},
            {Issue::WARN_USE_UNINITIALIZED_CC,          "WARN_USE_UNINITIALIZED_CC"},
            {Issue::WARN_READ_INST_AS_DATA,             "WARN_READ_INST_AS_DATA"},
            {Issue::WARN_POSSIBLE_WILD_WRITE,           "WARN_POSSIBLE_WILD_WRITE"},
            {Issue::WARN_WRITE_READ_ONLY_DATA,          "WARN_WRITE_READ_ONLY_DATA"},
            {Issue::WARN_MANUAL_HALT,                   "WARN_MANUAL_HALT"},
            {Issue::WARN_IMPROPER_RET,                  "WARN_IMPROPER_RET"},
            {Issue::WARN_REUSE_CODE_ACROSS_SUBROUTINES, "WARN_DUPLICATE_COLOR"},
            {Issue::WARN_HALT_IN_SUBROUTINE,            "WARN_HALT_IN_SUBROUTINE"},
            {Issue::ERR_COMPILE,                        "ERR_COMPILE"},
            {Issue::ERR_EXECUTE_DATA_AS_INST,           "ERR_EXECUTE_DATA_AS_INST"},
            {Issue::ERR_OVERWRITE_INST,                 "ERR_OVERWRITE_INST"},
            {Issue::ERR_SYMBOLIC_PC,                    "ERR_SYMBOLIC_PC"},
            {Issue::ERR_EXECUTE_UNINITIALIZED_MEMORY,   "ERR_EXECUTE_UNINITIALIZED_MEMORY"},
            {Issue::ERR_RET_IN_MAIN_CODE,               "ERR_RET_IN_MAIN_CODE"},
            {Issue::ERR_STATE_REACH_STEP_LIMIT,         "ERR_STATE_REACH_STEP_LIMIT"},
            {Issue::ERR_STATE_REACH_OUTPUT_LIMIT,       "ERR_STATE_REACH_OUTPUT_LIMIT"},
            {Issue::ERR_STATE_REACH_QUERY_LIMIT,        "ERR_STATE_REACH_QUERY_LIMIT"},
            {Issue::ERR_INCORRECT_OUTPUT,               "ERR_INCORRECT_OUTPUT"},
    };
    unordered_map<Issue::Type, string> issueDesc;
    unordered_map<Issue::Type, string> issueHint;
    unordered_map<Issue::Type, Issue::Level> issueLevel = {
            {Issue::WARN_COMPILE,                       Issue::WARNING},
            {Issue::WARN_POSSIBLE_WILD_READ,            Issue::WARNING},
            {Issue::WARN_READ_UNINITIALIZED_MEMORY,     Issue::WARNING},
            {Issue::WARN_USE_UNINITIALIZED_REGISTER,    Issue::WARNING},
            {Issue::WARN_USE_UNINITIALIZED_CC,          Issue::WARNING},
            {Issue::WARN_READ_INST_AS_DATA,             Issue::WARNING},
            {Issue::WARN_POSSIBLE_WILD_WRITE,           Issue::WARNING},
            {Issue::WARN_WRITE_READ_ONLY_DATA,          Issue::WARNING},
            {Issue::WARN_MANUAL_HALT,                   Issue::WARNING},
            {Issue::WARN_IMPROPER_RET,                  Issue::WARNING},
            {Issue::WARN_REUSE_CODE_ACROSS_SUBROUTINES, Issue::WARNING},
            {Issue::WARN_HALT_IN_SUBROUTINE,            Issue::WARNING},
            {Issue::ERR_COMPILE,                        Issue::ERROR},
            {Issue::ERR_EXECUTE_DATA_AS_INST,           Issue::ERROR},
            {Issue::ERR_OVERWRITE_INST,                 Issue::ERROR},
            {Issue::ERR_SYMBOLIC_PC,                    Issue::ERROR},
            {Issue::ERR_EXECUTE_UNINITIALIZED_MEMORY,   Issue::ERROR},
            {Issue::ERR_RET_IN_MAIN_CODE,               Issue::ERROR},
            {Issue::ERR_STATE_REACH_STEP_LIMIT,         Issue::ERROR},
            {Issue::ERR_STATE_REACH_OUTPUT_LIMIT,       Issue::ERROR},
            {Issue::ERR_STATE_REACH_QUERY_LIMIT,        Issue::ERROR},
            {Issue::ERR_INCORRECT_OUTPUT,               Issue::ERROR},
    };

    int definedIssueCount = Issue::PRE_DEFINED_ISSUE_COUNT;

    friend class IssueFilter;
};

}

#endif //KLC3_ISSUETRACKER_H
