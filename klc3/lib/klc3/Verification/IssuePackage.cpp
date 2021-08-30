//
// Created by liuzikai on 6/27/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Verification/IssuePackage.h"

#define REPORT_FIRST_DETECTION  0
#define AGGRESSIVE_FILTER_ISSUE  1  // if set, IssuePackage will only preserve the first detection of one issue

namespace klc3 {

IssuePackage::IssueInfo *IssuePackage::raiseIssue(Issue::Type type, const ref<MemValue> &location) {

    if (getIssueLevel(type) == Issue::NONE) return nullptr;

    Issue issue = {type, location};
    auto it = issues.find(issue);
    if (it == issues.end()) {
        it = issues.emplace(issue, vector<IssueInfo>{}).first;
#if REPORT_FIRST_DETECTION
        timedInfo() << "IssuePackage: First time detect ";
        writeIssueName(progInfo(), issue.type);
        if (!location.isNull()) progInfo() << " at " << location->sourceContext();
        progInfo() << "\n";
#endif
    }
#if AGGRESSIVE_FILTER_ISSUE
    else {
        // Discard the issue
        return nullptr;
    }
#endif
    it->second.emplace_back(IssueInfo{});
    return &it->second.back();
}

void IssuePackage::setIssueTypeLevel(Issue::Type type, Issue::Level level) {
    switch (type) {
        case Issue::NO_ISSUE:
            newProgErr() << "Cannot set level of NO_ISSUE\n";
            progExit();
            break;
        case Issue::WARN_COMPILE:
            newProgErr() << "Cannot set level of WARN_COMPILE\n";
            progExit();
            break;
        case Issue::WARN_POSSIBLE_WILD_READ:
        case Issue::WARN_READ_UNINITIALIZED_MEMORY:
        case Issue::WARN_USE_UNINITIALIZED_REGISTER:
        case Issue::WARN_USE_UNINITIALIZED_CC:
        case Issue::WARN_READ_INST_AS_DATA:
        case Issue::WARN_POSSIBLE_WILD_WRITE:
        case Issue::WARN_WRITE_READ_ONLY_DATA:
        case Issue::WARN_MANUAL_HALT:
        case Issue::WARN_IMPROPER_RET:
        case Issue::WARN_REUSE_CODE_ACROSS_SUBROUTINES:
        case Issue::WARN_HALT_IN_SUBROUTINE:
        case Issue::ERR_RET_IN_MAIN_CODE:
            // These issues accept all levels
            issueLevel[type] = level;
            break;
        case Issue::ERR_COMPILE:
        case Issue::ERR_INVALID_INST:
        case Issue::ERR_EXECUTE_DATA_AS_INST:
        case Issue::ERR_OVERWRITE_INST:
        case Issue::ERR_SYMBOLIC_PC:
        case Issue::ERR_EXECUTE_UNINITIALIZED_MEMORY:
        case Issue::ERR_STATE_REACH_STEP_LIMIT:
        case Issue::ERR_STATE_REACH_OUTPUT_LIMIT:
        case Issue::ERR_STATE_REACH_QUERY_LIMIT:
        case Issue::ERR_INCORRECT_OUTPUT:
            // These issues must be ERROR
            if (level != Issue::ERROR) {
                newProgErr() << "Issue ";
                writeIssueName(progErrs(), type);
                progErrs() << " must be ERROR\n";
                progExit();
            }
            issueLevel[type] = level;
            break;
        default:
            newProgErr() << "Unknown issue for SET_LEVEL\n";
            progExit();
            break;
    }
}

llvm::raw_ostream &IssuePackage::writeIssueName(llvm::raw_ostream &out, const Issue::Type &issueType) const {
    auto it = issueName.find(issueType);
    if (it != issueName.end()) {
        out << it->second;
        return out;
    }
    assert(!"Unknown issue type");
}

llvm::raw_ostream &IssuePackage::writeIssueDesc(llvm::raw_ostream &out, const Issue::Type &issueType) const {
    auto it = issueDesc.find(issueType);
    if (it != issueDesc.end()) {
        out << it->second;
        return out;
    }
    switch (issueType) {
        case Issue::WARN_COMPILE:
            out << "compile warning";
            break;
        case Issue::WARN_POSSIBLE_WILD_READ:
            out << "read a memory address outside the expected regions";
            break;
        case Issue::WARN_READ_UNINITIALIZED_MEMORY:
            out << "read uninitialized memory";
            break;
        case Issue::WARN_READ_INST_AS_DATA:
            out << "read a memory location which stores an instruction rather than data";
            break;
        case Issue::WARN_POSSIBLE_WILD_WRITE:
            out << "write to uninitialized memory";
            break;
        case Issue::WARN_WRITE_READ_ONLY_DATA:
            out << "overwrite read-only data";
            break;
        case Issue::WARN_MANUAL_HALT:
            out << "program halts, but no using the HALT TRAP";
            break;
        case Issue::ERR_COMPILE:
            out << "compile error";
            break;
        case Issue::ERR_EXECUTE_DATA_AS_INST:
            out << "execute data as an instruction";
            break;
        case Issue::ERR_OVERWRITE_INST:
            out << "overwrite an instruction";
            break;
        case Issue::ERR_SYMBOLIC_PC:
            out << "load test data into PC";
            break;
        case Issue::ERR_EXECUTE_UNINITIALIZED_MEMORY:
            out << "PC runs into uninitialized memory";
            break;
        case Issue::WARN_USE_UNINITIALIZED_REGISTER:
            out << "use uninitialized register";
            break;
        case Issue::WARN_USE_UNINITIALIZED_CC:
            out << "use uninitialized CC code";
            break;
        case Issue::WARN_IMPROPER_RET:
            out << "RET to improper location";
            break;
        case Issue::ERR_RET_IN_MAIN_CODE:
            out << "RET in main code";
            break;
        case Issue::WARN_REUSE_CODE_ACROSS_SUBROUTINES:
            out << "detect a code block shared by subroutine(s) and/or main code";
            break;
        case Issue::WARN_HALT_IN_SUBROUTINE:
            out << "HALT in subroutine";
            break;
        case Issue::ERR_STATE_REACH_STEP_LIMIT:
            out << "for some input, your program doesn't halt within given number of steps";
            break;
        case Issue::ERR_STATE_REACH_OUTPUT_LIMIT:
            out << "for some input, your program doesn't halt before it reaches the output limit";
            break;
        case Issue::ERR_STATE_REACH_QUERY_LIMIT:
            // It's actually not the time limit... But I don't want to explain query to students
            out << "for some input, your program doesn't halt before it reaches the time limit";
            break;
        case Issue::ERR_INCORRECT_OUTPUT:
            out << "incorrect output";
            break;
        default:
            // No description
            break;
    }
    return out;
}

string IssuePackage::getIssueHint(const Issue &issue) const {
    string out;
    auto it = issueHint.find(issue.type);
    if (it != issueHint.end()) {
        out += it->second;
        return out;
    }
    switch (issue.type) {
        case Issue::WARN_READ_UNINITIALIZED_MEMORY:
        case Issue::WARN_POSSIBLE_WILD_READ: {
            assert(issue.location->type == MemValue::MEM_INST && "Expecting issue happen at inst rather than data");
            InstValue *location = dyn_cast<InstValue>(issue.location);
            out += "Remember that uninitialized memory locations have undefined value. ";
            if (location->instID() == InstValue::LDR || location->instID() == InstValue::LDI) {
                // Reading dynamic address
                out += "Make sure you have calculated address correctly. ";
            }
            out += "If it is the location you want to read, make sure you have initialized it. ";
        }
            break;
        case Issue::WARN_READ_INST_AS_DATA:
            out += "It's possible that you have calculate the address incorrectly, "
                   "or maybe you don't preserve enough space for your data. ";
            break;
        case Issue::WARN_POSSIBLE_WILD_WRITE:
            out += "This is risky. "
                   "You are encouraged to use memory that is explicitly specified in your code (.BLKW, .FILL, etc.) or "
                   "explicitly allowed by the assignment. ";
            break;
        case Issue::WARN_WRITE_READ_ONLY_DATA:
            out += "You should not overwrite read-only data. ";
            break;
        case Issue::WARN_MANUAL_HALT:
            out += "Are you writing to machine control register directly? "
                   "If so, no a good style, use TRAP instead. "
                   "If not, probably you are writing to wrong address. ";
            break;
        case Issue::ERR_EXECUTE_DATA_AS_INST:
        case Issue::ERR_EXECUTE_UNINITIALIZED_MEMORY: {
            // Check the last executed instruction
            if (issue.location.isNull()) {
                // Don't have last executed instruction
                out += "Are you putting data at the location where your program starts executing???";
            } else {
                assert(issue.location->type == MemValue::MEM_INST && "Expecting issue happen at inst rather than data");
                InstValue *location = dyn_cast<InstValue>(issue.location);
                if (location->instID() == InstValue::JSR) {
                    out += "Are you jumping to correct label? ";
                } else if (location->instID() == InstValue::JMP) {
                    if (location->baseR() == Reg::R_R7) {  // RET
                        out += "Do you forget to restore R7 before RET? ";
                    } else {
                        out += "Have you calculate the address correctly? ";
                    }
                } else if (location->instID() == InstValue::JSRR) {
                    out += "Have you calculate the address correctly? ";
                } else {
                    out += "Do you forget RET or branching so that the program falls outside the range of "
                           "executable code? ";
                }
            }
        }
            break;
        case Issue::ERR_OVERWRITE_INST:
            out += "If you are not doing it deliberately, it's likely that you have made a mistake when calculating "
                   "the address to write to. If you are doing it deliberately, are you sure you need it in "
                   "this problem? ";
            // out += "If your instructors ask you to do so, tell them it's time to upgrade this tool :(";
            break;
        case Issue::ERR_SYMBOLIC_PC:
            out += "It's likely that you are jumping to an address which involves input data. ";
            // out += "If your instructors ask you to do so, tell them it's time to upgrade this tool :(";
            break;
        case Issue::WARN_USE_UNINITIALIZED_REGISTER:
            out += "Do not assume the initial value of register. "
                   "Now it is assumed to be 0 (notice: not necessarily when replay)";
            break;
        case Issue::WARN_USE_UNINITIALIZED_CC:
            out += "Do not assume the initial value of CC code. "
                   "Now it is assumed to be 0 (notice: not necessarily when replay)";
            break;
        case Issue::WARN_IMPROPER_RET:
            out += "Have you correctly restore R7? Or are you skipping levels deliberately? ";
            break;
        case Issue::ERR_RET_IN_MAIN_CODE:
            out += "Do not RET in main code. If you need to exit the program, use HALT. ";
            break;
        case Issue::WARN_REUSE_CODE_ACROSS_SUBROUTINES:
            out += "This issue is raised possibly because you reuse a piece of code by multiple subroutines and/or "
                   "the \"main\" (entry) code, using BR or JMP. It may work in LC-3 but you are not encouraged to do so. "
                   "If you want to reuse some code, make a subroutine. "
                   "Another possibility is that you RET incorrectly (not using RET but JMP or BR, or doesn't correctly "
                   "restore R7). ";
            break;
        case Issue::WARN_HALT_IN_SUBROUTINE:
            out += "It's a better practice to have a single HALT in main code and use return values for subroutines. ";
            break;
        case Issue::ERR_STATE_REACH_STEP_LIMIT:
            out += "Probably R7 is not restored in your subroutines or there is infinite loop in your code. "
                   "Notice that this doesn't means your program will never halt. But if a problem commonly requires "
                   "10 steps while your program requires 1000 steps, it's likely to have a bug. "
                   "Result comparison is not run until you fix this issue. ";
            break;
        case Issue::ERR_STATE_REACH_OUTPUT_LIMIT:
            out += "Probably R7 is not restored in your subroutines or there is infinite loop in your code. "
                   "Notice that your program already prints more characters to the screen than what is expected, "
                   "which can't be correct. Result comparison is not run until you fix this issue. ";
            break;
        case Issue::ERR_STATE_REACH_QUERY_LIMIT:
            out += "Probably R7 is not restored in your subroutines or there is infinite loop in your code. "
                   "Notice that this doesn't means your program will never halt. But if a problem commonly requires "
                   "1s while your program requires 100s, it's likely to have a bug. "
                   "Notice that this time is set on the feedback tool, which may not be the same when you run it. "
                   "Result comparison is not run until you fix this issue. ";
            break;
        case Issue::ERR_INCORRECT_OUTPUT:
            out += "Your output doesn't match the expected one. ";
            break;
        default:
            // No hint
            break;
    }
    return out;
}

Issue::Level IssuePackage::getIssueLevel(Issue::Type issueType) const {
    auto it = issueLevel.find(issueType);
    if (it != issueLevel.end()) {
        return it->second;
    }
    assert(!"Unknown issue type");
}

IssuePackage::IssueInfo *IssuePackage::newGlobalIssue(Issue::Type type, const ref<MemValue> &location) {
    return raiseIssue(type, location);
}

void IssuePackage::callBackForAllDesc(const map<const State *, Assignment> &assignments) {
    for (auto &it : issues) {
        for (auto &info: it.second) {
            if (info.descCallback != nullptr) {
                auto it2 = assignments.find(info.s);
                assert(it2 != assignments.end() && "No assignment for state found");
                info.descCallback(it.first, info.s, it2->second, info.descCallbackArg, info.note);
                info.descCallback = nullptr;
                info.descCallbackArg = nullptr;
            }
        }
    }
}

Issue::Type IssuePackage::registerIssue(const string &name, Issue::Level level) {
    auto type = static_cast<Issue::Type>(definedIssueCount++);
    issueName[type] = name;
    issueLevel[type] = level;
    return type;
}

Issue::Type IssuePackage::getIssueByName(const string &name) const {
    for (const auto &it : issueName) {
        if (it.second == name) return it.first;
    }
    return Issue::NO_ISSUE;
}

}