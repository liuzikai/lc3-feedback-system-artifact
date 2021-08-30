//
// Created by liuzikai on 8/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_ISSUEFILTER_H
#define KLC3_ISSUEFILTER_H

#include "klc3/Verification/IssuePackage.h"

namespace klc3 {

class IssueFilter {
public:

    IssuePackage filterIssues(const IssuePackage &input);

};

}

#endif //KLC3_ISSUEFILTER_H
