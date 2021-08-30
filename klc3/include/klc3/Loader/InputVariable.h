//
// Created by liuzikai on 1/7/21.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_INPUTVARIABLE_H
#define KLC3_INPUTVARIABLE_H

#include "klc3/Common.h"

namespace klc3 {

struct InputVariable {
    string name;
    uint16_t startAddr;
    uint16_t size;
    bool isSymbolic = false;
    bool isString = false;
    vector<uint16_t> fixedValues;
    const Array* array = nullptr;
};

struct InputFile {
    string filename;
    vector<string> commentLines;
    uint16_t startAddr;
    vector<InputVariable> variables;
};

}

#endif //KLC3_INPUTVARIABLE_H
