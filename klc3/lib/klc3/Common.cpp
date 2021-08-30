//
// Created by liuzikai on 3/22/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Common.h"

#include "klee/System/Time.h"
#include <iomanip>

namespace klc3 {

static struct HexTable {
    char tab[16];
    constexpr HexTable() : tab {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                'A', 'B', 'C', 'D', 'E', 'F'} {}
    constexpr char operator[](int val) const { return tab[val]; }
} constexpr hexTable;

llvm::cl::OptionCategory KLC3DebugCat("KLC3 debug options");

llvm::cl::opt<bool> TimedOutput(
        "timed-output",
        llvm::cl::desc("Print output with time stamp (usr time in ms, default=false)"),
        llvm::cl::init(false),
        llvm::cl::cat(KLC3DebugCat));

uint16_t castConstant(const ref<Expr> &value) {
    auto *CE = dyn_cast<ConstantExpr>(value);
    return CE->getZExtValue(Expr::Int16);
}

llvm::SmallString<4> to4DigitHex(uint16_t val) {
    llvm::SmallString<4> ret;
    ret.resize(4);
    ret[0] = hexTable[val >> 12];
    ret[1] = hexTable[(val >> 8) & 0xF];
    ret[2] = hexTable[(val >> 4) & 0xF];
    ret[3] = hexTable[val & 0xF];
    return ret;
}

llvm::SmallString<5> toLC3Hex(uint16_t val) {
    llvm::SmallString<5> ret;
    ret.resize(5);
    ret[0] = 'x';
    ret[1] = hexTable[val >> 12];
    ret[2] = hexTable[(val >> 8) & 0xF];
    ret[3] = hexTable[(val >> 4) & 0xF];
    ret[4] = hexTable[val & 0xF];
    return ret;
}

llvm::raw_ostream &newProgWarn() {
    llvm::errs().flush();
    return llvm::WithColor::warning();
}
llvm::raw_ostream &progWarns() {
    llvm::errs().flush();
    return llvm::errs();
}

llvm::raw_ostream &newProgErr() {
    llvm::errs().flush();
    return llvm::WithColor::error();
}
llvm::raw_ostream &progErrs() {
    llvm::outs().flush();
    llvm::errs().flush();
    return llvm::errs();
}

llvm::raw_ostream &progInfo() {
    llvm::outs().flush();
    llvm::errs().flush();
    return llvm::outs();
}

llvm::raw_ostream &timedInfo() {
    llvm::outs().flush();
    llvm::errs().flush();
    if (TimedOutput) {
        llvm::outs() << "[" << (int) (klee::time::getUserTime().toSeconds() * 1000) << "] ";
    }
    return llvm::outs();
}

void progExit(int exitCode) {
    exit(exitCode);
}


string strip(const string &in) {
    unsigned len = in.size();
    unsigned lead = 0, trail = len;
    while (lead < len && isspace(in[lead])) ++lead;
    while (trail > lead && isspace(in[trail - 1])) --trail;
    return in.substr(lead, trail - lead);
}

string rightPadding(const string& s, int w, char c) {
    std::stringstream ret;
    ret << std::right << std::setfill(c) << std::setw(w) << s;
    return ret.str();
}

string leftPadding(const string& s, int w, char c) {
    std::stringstream ret;
    ret << std::left << std::setfill(c) << std::setw(w) << s;
    return ret.str();
}

string floatToString(float val, int precision) {
    std::stringstream ret;
    ret << std::fixed << std::setprecision(precision) << val;
    return ret.str();
}

string escapeString(const string &str) {
    std::stringstream ret;
    for (char c : str) {
        if (c == '\n') ret << "\\n";
        else if (c == '\\') ret << "\\\\";
        else if (c == '"') ret << "\"";
        else ret << c;
    }
    return ret.str();
}

bool endswith(const string &str, const string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool startswith(const string &str, const string &prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

}