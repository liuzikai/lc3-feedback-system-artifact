//
// Created by liuzikai on 6/29/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLEE_ISSUE_H
#define KLEE_ISSUE_H

#include "klc3/Common.h"
#include "klc3/Core/MemoryValue.h"

namespace klc3 {

class Issue {
public:

    enum Type {
        NO_ISSUE,

        WARN_COMPILE,

        WARN_POSSIBLE_WILD_READ,
        WARN_READ_UNINITIALIZED_MEMORY,
        WARN_POSSIBLE_WILD_WRITE,
        WARN_WRITE_READ_ONLY_DATA,

        WARN_READ_INST_AS_DATA,

        WARN_USE_UNINITIALIZED_REGISTER,
        WARN_USE_UNINITIALIZED_CC,

        WARN_MANUAL_HALT,
        WARN_IMPROPER_RET,
        WARN_REUSE_CODE_ACROSS_SUBROUTINES,
        WARN_HALT_IN_SUBROUTINE,

        ERR_COMPILE,

        ERR_INVALID_INST,  // only for RTI, while executing data will fall to ERR_EXECUTE_DATA_AS_INST
        ERR_EXECUTE_DATA_AS_INST,
        ERR_OVERWRITE_INST,
        ERR_SYMBOLIC_PC,
        ERR_EXECUTE_UNINITIALIZED_MEMORY,
        ERR_RET_IN_MAIN_CODE,
        ERR_STATE_REACH_STEP_LIMIT,
        ERR_STATE_REACH_OUTPUT_LIMIT,
        ERR_STATE_REACH_QUERY_LIMIT,

        RUNTIME_ISSUE_COUNT,  // all issues above are raised during execution of test program and don't rely on gold

        ERR_INCORRECT_OUTPUT,  // this pre-defined issue and all user-defined issues can only be raised when comparing

        PRE_DEFINED_ISSUE_COUNT  // this and following indices will be used for user-defined issues

        // NOTE: when add/delete a build-in type issue, make sure to change IssuePackage as well
    };

    enum Level {
        ERROR,
        WARNING,
        NONE
    };

    /**
     * A unique issue is defined as a type of problem at a specific location.
     */

    Type type;
    ref<MemValue> location;  // can be nullptr

    bool operator==(const Issue &rhs) const {
        return (type == rhs.type && location.get() == rhs.location.get());
    }


};

struct IssueHasher {
    std::size_t operator()(const Issue &issue) const {
        return (reinterpret_cast<std::size_t>(issue.location.get()) << 5 | issue.type);
    }
};

struct IssueLocationLess {
    bool operator()(const Issue &l, const Issue &r) const {
        if (l.location.isNull() && !r.location.isNull()) return true;
        if (!l.location.isNull() && r.location.isNull()) return false;
        if (l.location.isNull() && r.location.isNull()) return (l.type < r.type);
        if (l.location->sourceFile == r.location->sourceFile) {
            if (l.location->sourceLine == r.location->sourceLine) {
                return (l.type < r.type);
            } else {
                return (l.location->sourceLine < r.location->sourceLine);
            }
        } else {
            return (l.location->sourceFile < r.location->sourceFile);
        }
    }
};

}

#endif //KLEE_ISSUE_H
