//
// Created by liuzikai on 9/14/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Generation/VariableInductor.h"

namespace klc3 {

Assignment VariableInductor::induceVariables(ConstraintSet constraints) const {
    // Need to make a copy in order to apply preferences

    ConstraintManager constraintManager(constraints);

    // Apply preferences
    for (const auto &it : preferences) {
        bool compatible = false;
        bool success = solver->mayBeTrue(Query(constraints, it), compatible);
        assert(success && "Unhandled solver error");
        if (compatible) {
            constraintManager.addConstraint(it);
        } else {
            // progInfo() << "A preference is unsatisfiable: " << it.first << "\n";
        }
    }

    vector<vector<unsigned char>> result;
    bool success = solver->getInitialValues(Query(constraints, builder->False()),
                                            arrays, result);
    assert(success && "Unhandled solver error");

    return {arrays, result};
}

}