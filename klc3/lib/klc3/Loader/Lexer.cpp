//
// Created by liuzikai on 1/14/21.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Loader/Loader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "klc3/Verification/IssuePackage.h"

namespace klc3 {

bool KLC3Loader::Lexer::load(const string &filename, bool isLC3OS, bool collectCompileIssues_) {

    loadingLC3OS = isLC3OS;
    curFilename = filename;
    collectCompileIssues = collectCompileIssues_;
    dataDefaultReadOnly = false;
    dataDefaultUninitialized = false;
    loadingInputFile = nullptr;
    lc3Symbols.clear();
    lc3LabelStrings.clear();

    std::ifstream asmFile(curFilename);
    if (!asmFile) {
        newProgErr() << "Failed to open " << curFilename << "\n";
        progExit();
    }

    if (ignoreKLC3Extension) puts("STARTING PASS 1");  // fully simulate lc3as behavior

    curPass = 1;
    curLine = 0;
    errCount = 0;
    sawORIG = sawEND = sawKLC3Command = 0;
    curAddr = 0x3000;
    newInstLine();
    switch_streams(&asmFile);

    yylex();

    if (sawORIG == 0) {
        if (!ignoreKLC3Extension && sawKLC3Command) {
            // File contains KLC3 commands but nothing else, OK
        } else {
            if (errCount == 0 && !sawEND) {
                reportCompileErr("file contains only comments");
            } else {
                if (sawEND == 0) {
                    reportCompileErr("no .ORIG or .END directive found");
                } else {
                    reportCompileErr("no .ORIG directive found");
                }
            }
            errCount++;
        }
    } else if (sawEND == 0) {
        reportCompileErr("no .END directive found");
        errCount++;
    }

    if (errCount > 0 || ignoreKLC3Extension) printf("%d errors found in first pass.\n", errCount);
    if (errCount > 0) {
        if (collectCompileIssues) {
            if (!isLC3OS && !loadingInputFile) {
                loadedPrograms.push_back(filename);
            }
            return false;
        } else {
            progExit();
        }
    }

    asmFile.clear();
    asmFile.seekg(0);
    yyrestart(&asmFile);
    /* Return lexer to initial state.  It is otherwise left in ls_finished if an .END directive was seen. */
    resetLexer();

    if (ignoreKLC3Extension) puts("STARTING PASS 2");  // fully simulate lc3as behavior

    curPass = 2;
    curLine = 0;
    errCount = 0;
    sawORIG = sawEND = 0;  // do nothing to sawKLC3Command
    curAddr = 0x3000;
    newInstLine();

    yylex();

    if (errCount > 0 || ignoreKLC3Extension) printf("%d errors found in second pass.\n", errCount);
    if (errCount > 0) {
        if (collectCompileIssues) {
            if (!isLC3OS && !loadingInputFile) {
                loadedPrograms.push_back(filename);
            }
            return false;
        } else {
            progExit();
        }
    }

    initPC = fileStartAddr;

    resetLexer();  // prepare for loading next file
    asmFile.close();

    if (!isLC3OS && !loadingInputFile) {
        loadedPrograms.push_back(filename);
    }

    return true;
}

void KLC3Loader::Lexer::newInstLine() {
    inst.op = OP_NONE;
    inst.ccode = CC_;
    curLine++;
    curLineContent.clear();
}

void KLC3Loader::Lexer::appendLineContent(const char *opname) {
    curLineContent += opname;
    if (curLineContent.back() == '\n' || curLineContent.back() == '\r') {
        curLineContent = strip(curLineContent);
    }
}

void KLC3Loader::Lexer::badOperands() {
    reportCompileErr(string("illegal operands for ") + OPNAMES[inst.op]);
    errCount++;
    newInstLine();
}

void KLC3Loader::Lexer::unterminatedString() {
    reportCompileErr("unterminated string");
    errCount++;
    newInstLine();
}

void KLC3Loader::Lexer::badLine() {
    reportCompileErr("contains unrecognizable characters");
    errCount++;
    newInstLine();
}

void KLC3Loader::Lexer::lineIgnored() {
    if (curPass == 1) {
        reportCompileWarn("all text after .END ignored");
    }
}

int KLC3Loader::Lexer::readVal(const char *s, int *vptr, int bits) {
    char *trash;
    long v;

    if (*s == 'x' || *s == 'X') {
        v = strtol(s + 1, &trash, 16);
    } else {
        if (*s == '#') {
            s++;
        }
        v = strtol(s, &trash, 10);
    }
    if (0x10000 > v && 0x8000 <= v) {
        v |= -65536L;   /* handles 64-bit longs properly */
    }
    if (v < -(1L << (bits - 1)) || v >= (1L << bits)) {
        reportCompileErr("constant outside of allowed range");
        errCount++;
        return -1;
    }
    if ((v & (1UL << (bits - 1))) != 0) {
        v |= ~((1UL << bits) - 1);
    }
    *vptr = v;
    return 0;
}

static char *sym_name(const char *name) {
    char *local = strdup(name);
    char *cut;

    /* Not fast, but no limit on label length...who cares? */
    for (cut = local; *cut != 0 && !isspace(*cut) && *cut != ':'; cut++);
    *cut = 0;

    return local;
}

int KLC3Loader::Lexer::findLabel(const char *optarg, int bits) {
    char *local;
    LC3Symbol *label;
    int limit, value;

    if (curPass == 1)
        return 0;

    local = sym_name(optarg);
    label = findSymbol(local);
    if (label != NULL) {
        value = label->addr;
        if (bits != 16) { /* Everything except 16 bits is PC-relative. */
            limit = (1L << (bits - 1));
            value -= curAddr + 1;
            if (value < -limit || value >= limit) {
                reportCompileErr(string("label \"") + local + "\" at distance " + std::to_string(value) +
                                 " (allowed range is " + std::to_string(-limit) +
                                 " to " + std::to_string(limit - 1) + ")");
                goto bad_label;
            }
            return value;
        }
        free(local);
        return label->addr;
    }
    reportCompileErr(string("unknown label \"") + local + "\"");

    bad_label:
    errCount++;
    free(local);
    return 0;
}

void KLC3Loader::Lexer::generateInstruction(OperandType operands, const char *opstr) {
    int val, r1, r2, r3;
    const char *o1;
    const char *o2;
    const char *o3;

    if ((OP_FORMAT_OK[inst.op] & (1UL << operands)) == 0) {
        badOperands();
        return;
    }
    o1 = opstr;
    while (isspace(*o1)) o1++;
    if ((o2 = strchr(o1, ',')) != NULL) {
        o2++;
        while (isspace(*o2)) o2++;
        if ((o3 = strchr(o2, ',')) != NULL) {
            o3++;
            while (isspace(*o3)) o3++;
        }
    } else {
        o3 = NULL;
    }
    if (inst.op == OP_ORIG) {
        if (sawORIG == 0) {
            if (readVal(o1, &curAddr, 16) == -1) {
                /* Pick a value; the error prevents code generation. */
                curAddr = 0x3000;
            }
            fileStartAddr = curAddr;
            sawORIG = 1;
        } else if (sawORIG == 1) {
            reportCompileErr("multiple .ORIG directives found");
            sawORIG = 2;
        }
        newInstLine();
        return;
    }
    if (sawORIG == 0) {
        reportCompileErr("instruction appears before .ORIG");
        errCount++;
        newInstLine();
        sawORIG = 2;
        return;
    }
    if ((PRE_PARSE[operands] & PP_R1) != 0) {
        r1 = o1[1] - '0';
    }
    if ((PRE_PARSE[operands] & PP_R2) != 0) {
        r2 = o2[1] - '0';
    }
    if ((PRE_PARSE[operands] & PP_R3) != 0) {
        r3 = o3[1] - '0';
    }
    if ((PRE_PARSE[operands] & PP_I2) != 0) {
        (void) readVal(o2, &val, 9);
    }
    if ((PRE_PARSE[operands] & PP_L2) != 0) {
        val = findLabel(o2, 9);
    }

    switch (inst.op) {
        /* Generate real instruction opcodes. */
        case OP_ADD:
            if (operands == O_RRI) {
                /* Check or read immediate range (error in first pass
                   prevents execution of second, so never fails). */
                (void) readVal(o3, &val, 5);
                writeInst(0x1020 | (r1 << 9) | (r2 << 6) | (val & 0x1F));
            } else {
                writeInst(0x1000 | (r1 << 9) | (r2 << 6) | r3);
            }
            break;
        case OP_AND:
            if (operands == O_RRI) {
                /* Check or read immediate range (error in first pass
                   prevents execution of second, so never fails). */
                (void) readVal(o3, &val, 5);
                writeInst(0x5020 | (r1 << 9) | (r2 << 6) | (val & 0x1F));
            } else {
                writeInst(0x5000 | (r1 << 9) | (r2 << 6) | r3);
            }
            break;
        case OP_BR:
            if (operands == O_I) {
                (void) readVal(o1, &val, 9);
            } else /* O_L */ {
                val = findLabel(o1, 9);
            }
            writeInst(inst.ccode | (val & 0x1FF));
            break;
        case OP_JMP:
            writeInst(0xC000 | (r1 << 6));
            break;
        case OP_JSR:
            if (operands == O_I) {
                (void) readVal(o1, &val, 11);
            } else /* O_L */{
                val = findLabel(o1, 11);
            }
            writeInst(0x4800 | (val & 0x7FF));
            break;
        case OP_JSRR:
            writeInst(0x4000 | (r1 << 6));
            break;
        case OP_LD:
            writeInst(0x2000 | (r1 << 9) | (val & 0x1FF));
            break;
        case OP_LDI:
            writeInst(0xA000 | (r1 << 9) | (val & 0x1FF));
            break;
        case OP_LDR:
            (void) readVal(o3, &val, 6);
            writeInst(0x6000 | (r1 << 9) | (r2 << 6) | (val & 0x3F));
            break;
        case OP_LEA:
            writeInst(0xE000 | (r1 << 9) | (val & 0x1FF));
            break;
        case OP_NOT:
            writeInst(0x903F | (r1 << 9) | (r2 << 6));
            break;
        case OP_RTI:
            writeInst(0x8000);
            break;
        case OP_ST:
            writeInst(0x3000 | (r1 << 9) | (val & 0x1FF));
            break;
        case OP_STI:
            writeInst(0xB000 | (r1 << 9) | (val & 0x1FF));
            break;
        case OP_STR:
            (void) readVal(o3, &val, 6);
            writeInst(0x7000 | (r1 << 9) | (r2 << 6) | (val & 0x3F));
            break;
        case OP_TRAP:
            (void) readVal(o1, &val, 8);
            writeInst(0xF000 | (val & 0xFF));
            break;

            /* Generate trap pseudo-ops. */
        case OP_GETC:
            writeInst(0xF020);
            break;
        case OP_HALT:
            writeInst(0xF025);
            break;
        case OP_IN:
            writeInst(0xF023);
            break;
        case OP_OUT:
            writeInst(0xF021);
            break;
        case OP_PUTS:
            writeInst(0xF022);
            break;
        case OP_PUTSP:
            writeInst(0xF024);
            break;

            /* Generate non-trap pseudo-ops. */
        case OP_FILL:
            if (operands == O_I) {
                (void) readVal(o1, &val, 16);
                val &= 0xFFFF;
            } else /* O_L */{
                val = findLabel(o1, 16);
            }
            writeFILL(val);
            break;
        case OP_RET:
            writeInst(0xC1C0);
            break;
        case OP_STRINGZ:
            writeSTRINGZ(o1);
            break;
        case OP_BLKW:
            (void) readVal(o1, &val, 16);
            val &= 0xFFFF;
            writeBLKW(val);
            break;

            /* Handled earlier or never used, so never seen here. */
        case OP_NONE:
        case OP_ORIG:
        case OP_END:
        case NUM_OPS:
            break;
    }
    newInstLine();
}

void KLC3Loader::Lexer::parseCCode(const char *ccstr) {
    if (*ccstr == 'N' || *ccstr == 'n') {
        inst.ccode |= CC_N;
        ccstr++;
    }
    if (*ccstr == 'Z' || *ccstr == 'z') {
        inst.ccode |= CC_Z;
        ccstr++;
    }
    if (*ccstr == 'P' || *ccstr == 'p') {
        inst.ccode |= CC_P;
    }

    /* special case: map BR to BRnzp */
    if (inst.ccode == CC_) {
        inst.ccode = CC_P | CC_Z | CC_N;
    }
}

void KLC3Loader::Lexer::foundLabel(const char *lname) {
    char *local = sym_name(lname);

    if (curPass == 1) {
        if (sawORIG == 0) {
            reportCompileErr("label appears before .ORIG");
            errCount++;
        } else if (addSymbol(local, curAddr) == -1) {
            reportCompileErr(string("label ") + local + " has already appeared");
            errCount++;
        }
    }

    free(local);
}

}