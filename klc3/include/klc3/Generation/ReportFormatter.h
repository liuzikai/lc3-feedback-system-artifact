//
// Created by liuzikai on 8/18/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_REPORTFORMATOR_H
#define KLC3_REPORTFORMATOR_H

#include "klc3/Verification/IssuePackage.h"
#include "klc3/Core/MemoryManager.h"

namespace klc3 {

extern llvm::cl::OptionCategory KLC3ReportCat;

class State;

class ReportFormatter {
public:

    virtual void generateReport(llvm::raw_ostream &out, const IssuePackage &issuePackage) = 0;

    /**
     * Get extension of the report file, including the dot (.txt, .md, etc.)
     * @return
     */
    virtual string getReportExtension() = 0;

    /// NOTE: all functions starting with "formatted" or "print" don't print extra linefeed after last line.

    virtual string formattedLocation(const ref <MemValue> &val) {
        return val->sourceFile + ":" + std::to_string(val->sourceLine);
    }

    virtual string formattedContext(const ref<MemValue> &val) {
        return "[" + val->sourceFile + ":" + std::to_string(val->sourceLine) + "] " + val->sourceContent;
    }

    /**
     * Print LC3 output with special format. There is no auto line feed at the end.
     * @param out
     * @param state
     * @param assignment
     * @return Whether it is a unprintable character
     */
    virtual bool printLC3Out(llvm::raw_ostream &out, const State* state, const Assignment &assignment);

    /// NOTE: all functions starting with "report" or "write" prints an extra linefeed after last line

    /**
     * Print a block of LC3Out. It's printLC3Out + "\n"
     * @param out
     * @param state
     * @param assignment
     * @return
     */
    virtual bool reportOutput(llvm::raw_ostream &out, const State* state, const Assignment &assignment);

    /**
     * Print comment for one or more output blocks
     * @param hasUnprintableCharacter
     */
    virtual void reportOutputComment(llvm::raw_ostream &out, bool hasUnprintableCharacter);

    /**
     * Print given register values. If a register is not initialized, print "<bits>".
     * @param out
     * @param state
     * @param assignment
     * @param regs
     */
    virtual void reportRegisters(llvm::raw_ostream &out, const State *state, const Assignment &assignment, const vector <Reg> &regs);

    /**
     * Print a region of memory
     * @param out
     * @param state
     * @param assignment
     * @param start
     * @param end
     */
    virtual void reportMemory(llvm::raw_ostream &out, const State* state, const Assignment &assignment, uint16_t start, uint16_t end);

    /**
     * Print the opening html for a "details" section. If not supported, just print the summary + ":\n"
     * @param out
     * @param summary
     */
    virtual void writeDetailsOpening(llvm::raw_ostream &out, const llvm::Twine &summary) { out << summary << ":\n"; }

    /**
     * Print the closing html for a "details section. If not supported, print nothing
     * @param out
     */
    virtual void writeDetailsClosing(llvm::raw_ostream &out) {}

    virtual ~ReportFormatter() = default;

protected:

    /**
     * Print a character with spacial format
     * @param out
     * @param value
     * @return Whether it is a unprintable character
     */
    virtual bool printCharacter(llvm::raw_ostream &out, uint16_t value);

};

class ReportFormatterBrief : public ReportFormatter {
public:

    void generateReport(llvm::raw_ostream &out, const IssuePackage &issuePackage) override;

    string getReportExtension() override { return ""; }
};

class ReportFormatterMarkdown : public ReportFormatter {
public:

    void generateReport(llvm::raw_ostream &out, const IssuePackage &issuePackage) override;

    string getReportExtension() override { return ".md"; }

    /// The following overriding function print things in markdown format

    bool reportOutput(llvm::raw_ostream &out, const State* state, const Assignment &assignment) override;

    void reportRegisters(llvm::raw_ostream &out, const State *state, const Assignment &assignment, const vector <Reg> &regs) override;

    void reportMemory(llvm::raw_ostream &out, const State *state, const Assignment &assignment, uint16_t start, uint16_t end) override;

    string formattedLocation(const ref <MemValue> &val) override;

    string formattedContext(const ref<MemValue> &val) override;

    void writeDetailsOpening(llvm::raw_ostream &out, const llvm::Twine &summary) override;

    void writeDetailsClosing(llvm::raw_ostream &out) override;

private:

    unordered_set<Issue::Type> commentGenerated;

};

extern std::unique_ptr<ReportFormatter> reportFormatter;

enum class ReportFormatterStyle {
    BRIEF,
    MARKDOWN
};

extern void createReportFormatter();

}

#endif //KLC3_REPORTFORMATOR_H
