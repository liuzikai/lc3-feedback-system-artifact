//
// Created by liuzikai on 3/22/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Core/MemoryValue.h"

#include <iostream>
#include <iomanip>

using namespace std;


namespace klc3 {

constexpr InstValue::InstDef InstValue::INST_DEF[];

bool InstValue::matchInst() {
    for (auto &i: INST_DEF) {
        if ((ir() & i.mask) == i.match) {
            def_ = i;
            return true;
        }
    }
    return false;
}

string InstValue::disassemble() const {

    static const char *const ccStr[8] = {
            "", "P", "Z", "ZP", "N", "NP", "NZ", "NZP"
    };

    ostringstream ret;
    ret << def_.name;
    if (def_.id == BR) ret << ccStr[cc()];
    ret << " ";

    int found = 0, tgt;
    if (def_.format & FMT_R1) {
        ret << (found ? "," : "") << "R" << dr();
        found = 1;
    }
    if (def_.format & FMT_R2) {
        ret << (found ? "," : "") << "R" << sr1();
        found = 1;
    }
    if (def_.format & FMT_R3) {
        ret << (found ? "," : "") << "R" << sr2();
        found = 1;
    }
    if (def_.format & FMT_IMM5) {
        ret << (found ? "," : "") << "#" << imm5();
        found = 1;
    }
    if (def_.format & FMT_IMM6) {
        ret << (found ? "," : "") << "#" << imm6();
        found = 1;
    }
    if (def_.format & FMT_VEC8) {
        ret << (found ? "," : "") << "x" << std::setfill('0') << std::setw(2) << std::hex << vec8();
        found = 1;
    }
    if (def_.format & FMT_ASC8) {
        ret << (found ? "," : "");
        found = 1;
        switch (vec8()) {
            case 7:
                ret << "'\\a'";
                break;
            case 8:
                ret << "'\\b'";
                break;
            case 9:
                ret << "'\\t'";
                break;
            case 10:
                ret << "'\\n'";
                break;
            case 11:
                ret << "'\\v'";
                break;
            case 12:
                ret << "'\\f'";
                break;
            case 13:
                ret << "'\\r'";
                break;
            case 27:
                ret << "'\\e'";
                break;
            case 34:
                ret << "'\\\"'";
                break;
            case 44:
                ret << "'\\''";
                break;
            case 92:
                ret << "'\\\\'";
                break;
            default:
                if (isprint(vec8()))
                    ret << "'" << (char) vec8() << "'";
                else
                    ret << "x" << std::setfill('0') << std::setw(2) << std::hex << vec8();
                break;
        }
    }
    if (def_.format & FMT_IMM9) {
        ret << (found ? "," : "");
        found = 1;
        tgt = (addr + 1 + imm9()) & 0xFFFF;
        ret << "x" << std::setfill('0') << std::setw(4) << std::hex << tgt;
        // Not supporting translate to label yet
    }
    if (def_.format & FMT_IMM11) {
        ret << (found ? "," : "");
        found = 1;
        tgt = (addr + 1 + imm11()) & 0xFFFF;
        ret << "x" << std::setfill('0') << std::setw(4) << std::hex << tgt;
        // Not supporting translate to label yet
    }
    if (def_.format & FMT_IMM16) {
        ret << (found ? "," : "");
        found = 1;
        ret << "x" << std::setfill('0') << std::setw(4) << std::hex << ir();
        // Not supporting translate to label yet
    }

    return ret.str();
}

}