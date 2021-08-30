//
// Created by liuzikai on 8/18/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Generation/ReportFormatter.h"
#include "klc3/Generation/ResultGenerator.h"

namespace klc3 {

llvm::cl::OptionCategory KLC3ReportCat(
        "KLC3 report options",
        "These options control KLC3 report.");

llvm::cl::opt<ReportFormatterStyle> ReportStyle(
        "report-style",
        llvm::cl::desc("Report format style (default=markdown)"),
        llvm::cl::values(
                clEnumValN(ReportFormatterStyle::BRIEF, "brief",
                           "Only name and location, usually for machine processing"),
                clEnumValN(ReportFormatterStyle::MARKDOWN, "markdown", "GitHub preferred markdown")
        ),
        llvm::cl::init(ReportFormatterStyle::MARKDOWN),
        llvm::cl::cat(KLC3ReportCat));

llvm::cl::opt<string> ReportRelativePath(
        "report-relative-path",
        llvm::cl::desc(
                "Relative path from report to actual output directory, which will be used for URL. (default="", should ends with /)"),
        llvm::cl::init(""),
        llvm::cl::cat(KLC3ReportCat));

llvm::cl::opt<bool> ReportUseURL(
        "report-use-url",
        llvm::cl::desc("Generate links in the report (for markdown style, default=true)"),
        llvm::cl::init(true),
        llvm::cl::cat(KLC3ReportCat));

llvm::cl::opt<bool> OutputReplaceSpace(
        "report-replace-space",
        llvm::cl::desc("Replace each space with '_' for LC-3 output in the report (default=false)"),
        llvm::cl::init(false),
        llvm::cl::cat(KLC3ReportCat));

llvm::cl::opt<bool> OutputReplaceLinefeed(
        "report-replace-linefeed",
        llvm::cl::desc(
                "Replace '\\n' and '\\r' with unicode special character for LC-3 output in the report (default=false)"),
        llvm::cl::init(false),
        llvm::cl::cat(KLC3ReportCat));

std::unique_ptr<ReportFormatter> reportFormatter;

void createReportFormatter() {
    switch (ReportStyle) {
        case ReportFormatterStyle::BRIEF:
            reportFormatter = std::make_unique<ReportFormatterBrief>();
            break;
        case ReportFormatterStyle::MARKDOWN:
            reportFormatter = std::make_unique<ReportFormatterMarkdown>();
            break;
    }
}

bool ReportFormatter::printLC3Out(llvm::raw_ostream &out, const State *state, const Assignment &assignment) {
    bool hasUnprintableCharacter = false;
    for (const auto &val : state->lc3Out) {
        uint16_t c;
        if (val->getKind() == klee::Expr::Constant) {
            c = castConstant(val);
        } else {
            c = castConstant(assignment.evaluate(val));
        }
        if (printCharacter(out, c)) {
            hasUnprintableCharacter = true;
        }
    }
    if (OutputReplaceLinefeed) {
        out << "\u23CE";
        out << "\n";
    }
    return hasUnprintableCharacter;
}

bool ReportFormatter::printCharacter(llvm::raw_ostream &out, uint16_t value) {
    if (OutputReplaceLinefeed) {
        if (value == '\n' || value == '\r') {
            out << "\u23CE";
        }
    }
    if ((value > 31 && value < 127) || (value == 13) || (value == 10)) {
        // Printable character
        if (value == ' ' && OutputReplaceSpace) {
            out << '_';
        } else {
            out << (char) value;
        }
        return false;
    } else {
        // Unprintable character, print in the format of "\xABCD"
        out << "\\" << toLC3Hex(value);
        return true;
    }
}

bool ReportFormatter::reportOutput(llvm::raw_ostream &out, const State *state, const Assignment &assignment) {
    bool ret = printLC3Out(out, state, assignment);
    out << "\n";
    return ret;
}

void ReportFormatter::reportOutputComment(llvm::raw_ostream &out, bool hasUnprintableCharacter) {
    if (OutputReplaceLinefeed) out << "`\u23CE` represents linefeed. ";
    if (OutputReplaceSpace) {
        out << "Spaces are shown as `_`. ";
    } else {
        out << "Spaces may not be shown clearly. Selecting the text may help. ";
    }

    if (hasUnprintableCharacter) {
        out << "` \\xABCD` means that you try to print value 0xABCD, which is not a printable ASCII. ";
    }
    out << "\n";
}

void ReportFormatter::reportRegisters(llvm::raw_ostream &out, const State *state, const Assignment &assignment,
                                      const vector<Reg> &regs) {
    for (Reg r : regs) {
        const ref<Expr> &e = state->getReg(r);
        out << "R" << (int) r << "=";
        if (e.isNull()) {
            out << "<bits>" << " ";
        } else {
            out << toLC3Hex(castConstant(assignment.evaluate(e))) << "  ";
        }
    }
    out << "\n";
}

void ReportFormatter::reportMemory(llvm::raw_ostream &out, const State *state, const Assignment &assignment,
                                   uint16_t addrStart, uint16_t addrEnd) {
    // Reference: lc3sim.c, not support wrap over
    // addrEnd is inclusive
    for (unsigned long start = (addrStart / 12) * 12; start <= addrEnd; start += 12) {
        out << to4DigitHex(start) << ": ";
        for (unsigned long i = 0, addr = start; i < 12; i++, addr++) {
            if (addr >= addrStart && addr <= addrEnd) {
                ref<MemValue> val = state->mem.read(addr);
                if (val.isNull() || val->e.isNull()) {  // unspecified memory location or uninitialized value
                    out << "bits ";
                } else {
                    out << to4DigitHex(castConstant(assignment.evaluate(val->e))) << " ";
                }
            } else {
                out << "     ";
            }
        }
        out << " ";
        for (unsigned long i = 0, addr = start; i < 12; i++, addr++) {
            if (addr >= addrStart && addr <= addrEnd) {
                ref<MemValue> val = state->mem.read(addr);
                if (val.isNull() || val->e.isNull()) {  // unspecified memory location or uninitialized value
                    out << '.';
                } else {
                    uint16_t num = castConstant(assignment.evaluate(val->e));
                    if (num < 0x100 && isprint(num)) {
                        out << static_cast<char>(num);
                    } else {
                        out << '.';
                    }
                }
            }
        }
        out << "\n";
    }
}

void ReportFormatterBrief::generateReport(llvm::raw_ostream &out, const IssuePackage &issuePackage) {
    for (const auto &it : issuePackage.getIssues()) {
        issuePackage.writeIssueName(out, it.first.type);
        if (!it.first.location.isNull()) {
            out << " " << it.first.location->sourceContext();
        }
        out << "\n";
    }
}

void ReportFormatterMarkdown::generateReport(llvm::raw_ostream &out, const IssuePackage &issuePackage) {

    for (const auto &it : issuePackage.getIssues()) {

        const auto &issue = it.first;
        const vector<IssuePackage::IssueInfo> &infos = it.second;

        string issuePrefix, issueEmoji;
        switch (issuePackage.getIssueLevel(issue.type)) {
            case Issue::WARNING:
                issuePrefix = "**WARNING**: ";
                issueEmoji = ":warning:";
                break;
            case Issue::ERROR:
                issuePrefix = "**ERROR**: ";
                issueEmoji = ":boom:";
                break;
            case Issue::NONE:
                assert(!"NONE level issue found in IssuePackage");
        }
        out << issueEmoji << " ";
        out << issuePrefix;

        issuePackage.writeIssueDesc(out, issue.type);  // ends with neither period nor space
        if (!issue.location.isNull()) {
            out << " at " << formattedContext(issue.location);
        }
        out << ".\n"  // end of issue desc or location
            << "\n";  // paragraph

        // We trust that input issuePackage has been filtered, so there won't be too much info

        for (const auto &info : infos) {
            if (!info.note.empty()) {
                out << "**NOTE**: ";
                if (info.s != nullptr) {
                    if (!info.generatedCaseName.empty()) {
                        out << "in [" << info.generatedCaseName << "]("
                            << ReportRelativePath << info.generatedCaseName << "), ";
                    } else {
                        assert(!"A state mentioned in the report is not generated");
                        // out << "in _[a state not generated]_, ";
                    }
                }
                if (info.stepCount != -1) {
                    out << "at step " << info.stepCount << ", ";
                }
                out << info.note;  // note should have a linefeed at the end of last line

                out << "\n";  // paragraph
            }
        }

        if (commentGenerated.find(issue.type) == commentGenerated.end()) {
            string remark = issuePackage.getIssueHint(issue);
            if (!remark.empty()) {
                out << "**REMARK**: " << remark << "\n"
                    << "\n";  // paragraph
            }
            commentGenerated.insert(issue.type);
        }

        out << "<br><br>\n\n";  // a paragraph of <br><br>
    }

}

string ReportFormatterMarkdown::formattedLocation(const ref<MemValue> &val) {
    // Print in markdown format
    if (ReportUseURL) {
        return "[" + val->sourceFile + ":" + std::to_string(val->sourceLine) + "](" +
               ReportRelativePath + val->sourceFile + "#L" + std::to_string(val->sourceLine) + ")";
    } else {
        return ReportFormatter::formattedLocation(val);
    }
}

string ReportFormatterMarkdown::formattedContext(const ref<MemValue> &val) {
    // Print in markdown format
    if (ReportUseURL) {
        return "[[" + val->sourceFile + ":" + std::to_string(val->sourceLine) + "](" +
               ReportRelativePath + val->sourceFile + "#L" + std::to_string(val->sourceLine) +
               ")] `" + val->sourceContent + "`";
    } else {
        return ReportFormatter::formattedContext(val);
    }
}

bool ReportFormatterMarkdown::reportOutput(llvm::raw_ostream &out, const State *state, const Assignment &assignment) {
    // Print in markdown format
    out << "```\n";
    bool ret = ReportFormatter::reportOutput(out, state, assignment);  // there is a linefeed after last line
    out << "```\n";
    return ret;
}


void ReportFormatterMarkdown::reportRegisters(llvm::raw_ostream &out, const State *state,
                                              const Assignment &assignment, const vector<Reg> &regs) {
    out << "```\n";
    ReportFormatter::reportRegisters(out, state, assignment, regs);  // there is a linefeed after last line
    out << "```\n";
}

void ReportFormatterMarkdown::reportMemory(llvm::raw_ostream &out, const State *state, const Assignment &assignment,
                                           uint16_t start, uint16_t end) {
    out << "```\n";
    ReportFormatter::reportMemory(out, state, assignment, start, end);  // there is a linefeed after last line
    out << "```\n";
}

void ReportFormatterMarkdown::writeDetailsOpening(llvm::raw_ostream &out, const llvm::Twine &summary) {
    out << "<details><summary>" << summary << " (click to expand) </summary>\n\n<br>\n\n";
}

void ReportFormatterMarkdown::writeDetailsClosing(llvm::raw_ostream &out) {
    out << "</details>\n";
}

}