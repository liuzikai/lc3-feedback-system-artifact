//===-- Assignment.cpp ----------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/Assignment.h"

namespace klee {

void Assignment::dump() {
  if (bindings.size() == 0) {
    llvm::errs() << "No bindings\n";
    return;
  }
  for (bindings_ty::iterator i = bindings.begin(), e = bindings.end(); i != e;
       ++i) {
    llvm::errs() << (*i).first->name << "\n[";
    for (int j = 0, k = (*i).second.size(); j < k; ++j) {
        // NOTE: [liuzikai] support range size different than Int8 by combining bytes
        unsigned wordSize = (*i).first->range / 8;
        unsigned long long value = 0;
        for (unsigned b = 0; b < wordSize; b++) value |= ((*i).second[j * wordSize + b] << (8U * b));
      llvm::errs() << (int)value << ",";
  }
    llvm::errs() << "]\n";
  }
}

ConstraintSet Assignment::createConstraintsFromAssignment() const {
  ConstraintSet result;
  for (const auto &binding : bindings) {
    const auto &array = binding.first;
    const auto &values = binding.second;

    for (unsigned arrayIndex = 0; arrayIndex < array->size; ++arrayIndex) {
        // NOTE: [liuzikai] support range size different than Int8 by combining bytes
        unsigned wordSize = array->range / 8;
        unsigned long long value = 0;
        for (unsigned k = 0; k < wordSize; k++) value |= (values[arrayIndex * wordSize + k] << (8U * k));
      result.push_back(EqExpr::create(
          ReadExpr::create(UpdateList(array, 0),
                           ConstantExpr::alloc(arrayIndex, array->getDomain())),
          ConstantExpr::alloc(value, array->getRange())));
    }
  }
  return result;
}
}
