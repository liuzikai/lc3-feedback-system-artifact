//
// Created by liuzikai on 9/14/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_VARIABLEINDUCTOR_H
#define KLC3_VARIABLEINDUCTOR_H

#include "klc3/Common.h"

namespace klc3 {

class VariableInductor : protected WithBuilder {
public:

    VariableInductor(ExprBuilder *builder, Solver *solver, vector<const Array *> arrays, vector<ref<Expr>> preferences)
            : WithBuilder(builder), solver(solver), arrays(std::move(arrays)), preferences(std::move(preferences)) {}

    // Need to make a copy in order to apply preferences
    Assignment induceVariables(ConstraintSet constraints) const;

private:

    Solver *solver;
    vector<const Array *> arrays;
    vector<ref<Expr>> preferences;

};

}

#endif //KLC3_VARIABLEINDUCTOR_H
