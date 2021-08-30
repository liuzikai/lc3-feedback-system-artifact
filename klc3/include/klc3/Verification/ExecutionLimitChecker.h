//
// Created by liuzikai on 12/26/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_EXECUTIONLIMITCHECKER_H
#define KLC3_EXECUTIONLIMITCHECKER_H

#include "klc3/Core/State.h"

namespace klc3 {

extern llvm::cl::OptionCategory KLC3ExecutionCat;

class ExecutionLimitChecker {
public:

    static void checkLimitOnState(State *s);

    static void appendOutputToIssueDesc(const Issue &issue, State *s, const Assignment &assignment, void *arg, string &desc);

};

}


#endif //KLC3_EXECUTIONLIMITCHECKER_H
