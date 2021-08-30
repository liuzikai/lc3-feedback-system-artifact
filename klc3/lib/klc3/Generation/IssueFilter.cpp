//
// Created by liuzikai on 8/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Generation/IssueFilter.h"

namespace klc3 {

IssuePackage IssueFilter::filterIssues(const IssuePackage &input) {

    // Make a copy of the IssuePackage
    IssuePackage ret = input;

    for (auto &it : ret.issues) {

        vector<IssuePackage::IssueInfo> &infos = it.second;

        // For each issue, select only one with least step
        if (!infos.empty()) {
            std::sort(infos.begin(), infos.end(), IssuePackage::IssueStepLess());
            auto selectedInfo = infos[0];
            infos.clear();
            infos.push_back(selectedInfo);
        }

    }

    return ret;
}

}