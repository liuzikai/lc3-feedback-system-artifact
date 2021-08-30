//
// Created by liuzikai on 3/20/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_COMMON_H
#define KLC3_COMMON_H

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprBuilder.h"
#include "klee/Expr/Constraints.h"
#include "klee/Expr/Assignment.h"
#include "klee/Solver/Solver.h"

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <stack>
#include <deque>
#include <utility>
#include <array>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/WithColor.h"

namespace klc3 {

using klee::ref;
using klee::dyn_cast;
using klee::Expr;
using klee::Array;
using klee::UpdateList;
using klee::UpdateNode;
using klee::ConstantExpr;
using klee::ExprBuilder;
using klee::Solver;
using klee::Query;
using klee::ConstraintManager;
using klee::ConstraintSet;
using klee::Assignment;

using std::string;
using std::vector;
using std::list;
using std::map;
using std::unordered_map;
using std::set;
using std::unordered_set;
using std::stack;
using std::pair;
using std::stringstream;
using std::deque;
using std::array;

using llvm::StringRef;
using llvm::SmallVector;
using llvm::SmallString;

class State;
using StateVector = llvm::SmallVector<State *, 4>;
using ConstStateVector = llvm::SmallVector<const State *, 4>;

extern llvm::cl::OptionCategory KLC3DebugCat;

uint16_t castConstant(const ref<Expr> &value);

enum Reg {
    R_R0 = 0, R_R1, R_R2, R_R3, R_R4, R_R5, R_R6, R_R7,
    R_PC, R_IR, R_PSR,
    NUM_REGS
};

class WithBuilder {
protected:

    WithBuilder(ExprBuilder *builder) : builder(builder) {};

    ExprBuilder *builder;

    // There should be no actual bit change with assignments between signed and unsigned int
    ref<ConstantExpr> buildConstant(int16_t value) const {
        static ref<ConstantExpr> zero = builder->Constant(0, Expr::Int16);
        if (value == 0) return zero;
        else return builder->Constant(value, Expr::Int16);
    }
};

llvm::SmallString<4> to4DigitHex(uint16_t val);

llvm::SmallString<5> toLC3Hex(uint16_t val);

template<template<typename = string, typename = std::allocator<string> > class Container>
string join(const string &delimiter, const Container<string> &p) {
    if (p.empty()) return "";
    std::stringstream ret;
    for (unsigned i = 0; i < p.size(); i++) {
        if (i != 0) ret << delimiter;
        ret << p[i];
    }
    return ret.str();
}

string strip(const string &in);

string rightPadding(const string &s, int w, char c);

string leftPadding(const string &s, int w, char c);

string floatToString(float val, int precision);

string escapeString(const string &str);

using PathString = llvm::SmallString<128>;

bool startswith(const string &str, const string &prefix);

bool endswith(const string &str, const string &suffix);

llvm::raw_ostream &newProgWarn();

llvm::raw_ostream &progWarns();

llvm::raw_ostream &newProgErr();

llvm::raw_ostream &progErrs();

llvm::raw_ostream &progInfo();

llvm::raw_ostream &timedInfo();

void progExit(int exitCode = 1);

}

#endif //KLEE_COMMON_H
