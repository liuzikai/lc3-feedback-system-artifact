//
// Created by liuzikai on 3/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Loader/Loader.h"
#include "klc3/Verification/IssuePackage.h"
#include "klc3/Verification/CrossChecker.h"
#include "llvm/Support/CommandLine.h"
#include <fstream>

using namespace std;

namespace klc3 {

llvm::cl::opt<bool> UseFileBasename(
        "use-file-basename",
        llvm::cl::desc(
                "Use file basename (filename only) for source context (default=true)"),
        llvm::cl::init(true));

constexpr const char *KLC3Loader::Lexer::OPNAMES[];
constexpr int KLC3Loader::Lexer::OP_FORMAT_OK[];
constexpr unsigned KLC3Loader::Lexer::PRE_PARSE[];


KLC3Loader::KLC3Loader(ExprBuilder *builder, ArrayCache *arrayCache, IssuePackage *issuePackage,
                       CrossChecker *crossChecker, bool ignoreKLC3Extension)
        : WithBuilder(builder), arrayCache(arrayCache), issuePackage(issuePackage), crossChecker(crossChecker),
          ignoreKLC3Extension(ignoreKLC3Extension) {}

void KLC3Loader::loadLC3OS(const string &filename) {
    load(filename, true, false, false);
    initPC = 0x200;
}

bool KLC3Loader::load(const string &filename, bool isLC3OS, bool allowInputFile, bool collectCompileIssues) {
    auto lexer = std::make_unique<Lexer>(builder, arrayCache, issuePackage, crossChecker,
                                         symbols, mem, constraints, preferences, initPC,
                                         loadedPrograms, inputFiles, allowInputFile, ignoreKLC3Extension);
    return lexer->load(filename, isLC3OS, collectCompileIssues);
}

void KLC3Loader::Lexer::processConstraintLine(const string &line) {
    std::vector<std::string> fields;
    splitString(line, fields, ' ');

    if (fields.size() == 3 && fields[1] == "is") {  // constraint of define type
        processDefiningConstraint(fields);
    } else {  // adding constraints
        processNumericalConstraint(line);
    }
}

void KLC3Loader::Lexer::splitNameAndSubscript(const string &s, string &name, uint16_t &size) {
    auto idx = s.find('[');
    assert(idx != std::string::npos && "No subscript");
    name = s.substr(0, idx);
    size = std::stoi(s.substr(idx + 1, s.length() - idx - 2));
}

void KLC3Loader::Lexer::splitString(const string &str, vector<string> &result, const char delimiter) {
    result.clear();
    stringstream lineStream(str);
    string tempStr;
    while (getline(lineStream, tempStr, delimiter)) {
        result.push_back(tempStr);
    }
}

KLC3Loader::SymbolInfo &KLC3Loader::Lexer::defineMemSymbol(string nameAndSize, bool forRead, bool forWrite) {
    // Define memory address as symbolic array

    uint16_t start = curAddr;
    if (!endswith(nameAndSize, "]")) nameAndSize += "[1]";   // add size "[1]"

    string name;
    uint16_t size;
    splitNameAndSubscript(nameAndSize, name, size);

    // Check for duplication
    if (symbols.find(name) != symbols.end()) {
        reportKLC3ErrExit("Symbolic variable name \"" + name + "\" is not unique");
    }

    // Output info
    progInfo() << "Create symbol \"" << name << "\" ";
    if (size == 1) {
        progInfo() << "at addr " << toLC3Hex(start) << "\n";
    } else {
        progInfo() << "[" << size << "] among addr " << toLC3Hex(start) << " to " << toLC3Hex(start + size - 1) << "\n";
    }

    // Create new Array and corresponding empty UpdateList
    const Array *array = arrayCache->CreateArray(name, size, nullptr, nullptr,
                                                 Expr::Int16, Expr::Int16);
    UpdateList updateList(array, nullptr);
    // UpdateList is not stored or passed to Executor, as it is stored in the ReadExpr constructed as follows

    symbols.emplace(name, SymbolInfo{name, size, start, array});

    // Construct ReadExpr in memory
    for (unsigned long offset = 0; offset < size; offset++) {
        if (mem.find(start + offset) != mem.end()) {
            auto originalValue = mem[start + offset];
            if (originalValue->type == MemValue::MEM_INST) {
                reportKLC3ErrExit("Symbolic variable name \"" + name + "\" is overwriting instruction" + "\n" +
                                  "  Overwritten: " + originalValue->sourceContext());
            } else if (originalValue->e->getKind() == Expr::Read) {
                reportKLC3ErrExit("Redefining symbol on at address" + toLC3Hex(start + offset) + "\n" +
                                  "  Overwritten: " + originalValue->sourceContext());
            }
        }
        ref<DataValue> v = DataValue::alloc(start + offset,
                                            builder->Read(updateList, builder->Constant(offset, Expr::Int16)),
                                            forRead, forWrite);
        mem.emplace(start + offset, v);
    }

    return symbols[name];
}

void KLC3Loader::Lexer::processDefiningConstraint(const vector<string> &fields) {
    assert(fields.size() == 3 && fields[1] == "is");

    // Parse parameters
    string nameAndSize = fields[0];
    if (!endswith(nameAndSize, "]")) nameAndSize += "[0]";   // add index "0"

    string name;
    uint16_t index;
    splitNameAndSubscript(nameAndSize, name, index);

    // Look up corresponding symbol
    if (symbols.find(name) == symbols.end()) {
        reportKLC3ErrExit("Unknown symbol " + name);
    }

    const SymbolInfo &symbol = symbols[name];

    if (index >= symbol.array->size) {
        reportKLC3ErrExit(nameAndSize + " is out of range of " + name +
                          " (size: " + std::to_string(symbol.array->size) + ")");
    }


    if (fields[2] == "fixed-length-string" || fields[2] == "var-length-string") {
        addConstraintOfString(symbol, (fields[2] == "var-length-string"));
    } else {
        reportKLC3ErrExit("Unknown constraint type " + fields[2]);
    }
}

void KLC3Loader::Lexer::addConstraintOfString(const SymbolInfo &symbol, bool variableLength) {

    for (unsigned long index = 0; index < symbol.array->size - 1; index++) {

        // Only using Eq, Slt and Sle in canonical mode
        ref<Expr> entry = mem[symbol.startAddr + index]->e;

        ref<Expr> constraint;

        // Printable character
        ref<Expr> upper = builder->Sle(entry, buildConstant(0x7E));  // 0x7E is ~
        ref<Expr> lower = builder->Sle(buildConstant(0x20), entry);  // 0x20 is space
        constraint = builder->And(upper, lower);

        if (variableLength) {
            constraint = builder->Or(constraint, builder->Eq(entry, buildConstant(0)));
        }
        constraints.push_back(constraint);

        // Prefer characters from A to Z
        ref<Expr> preference = builder->And(
                builder->Sle(buildConstant(charPreferenceLowerBound), entry),
                builder->Sle(entry, buildConstant(0x5A))
        );
        preferences.push_back(preference);
    }

    // The last character must be null-terminated
    ref<Expr> constraint = builder->Eq(mem[symbol.startAddr + symbol.array->size - 1]->e, buildConstant(0));
    constraints.push_back(constraint);

    if (symbol.inputVariable) {
        symbol.inputVariable->isString = true;
    }

    ++charPreferenceLowerBound;
    if (charPreferenceLowerBound > 0x5A) charPreferenceLowerBound = 0x41;  // goes over Z, go back to A
}

void KLC3Loader::Lexer::processNumericalConstraint(const string &line) {

    ref<Expr> constraint = nullptr;

    vector<string> orTokens;
    splitString(line, orTokens, '|');
    for (string orToken : orTokens) {
        orToken = strip(orToken);

        ref<Expr> orSubConstraint = nullptr;

        vector<string> andTokens;
        splitString(orToken, andTokens, '&');

        for (string andToken : andTokens) {
            andToken = strip(andToken);

            vector<string> fields;
            splitString(andToken, fields, ' ');

            ref<Expr> andSubConstraint = processSingleConstraint(fields);

            if (orSubConstraint.isNull()) {
                orSubConstraint = andSubConstraint;
            } else {
                orSubConstraint = builder->And(orSubConstraint, andSubConstraint);
            }

        }

        if (constraint.isNull()) {
            constraint = orSubConstraint;
        } else {
            constraint = builder->Or(constraint, orSubConstraint);
        }
    }

    constraints.push_back(constraint);
}

ref<Expr> KLC3Loader::Lexer::processSingleConstraint(const vector<string> &fields) {

    if (fields.size() != 3) {
        reportKLC3ErrExit("Invalid line in var file: ");
    }

    // Parse parameters
    string nameAndSize = fields[0];
    if (!endswith(nameAndSize, "]")) nameAndSize += "[0]";   // add index "0"

    string name;
    uint16_t index;
    splitNameAndSubscript(nameAndSize, name, index);

    // Look up corresponding symbol
    if (symbols.find(name) == symbols.end()) {
        reportKLC3ErrExit("Unknown symbol " + name);
    }

    const SymbolInfo &symbol = symbols[name];

    if (index >= symbol.array->size) {
        reportKLC3ErrExit(nameAndSize + " is out of range of " + name +
                          " (size: " + std::to_string(symbol.array->size) + ")");
    }

    return constructSingleConstraint(symbol, index, fields[1], fields[2]);
}

ref<Expr> KLC3Loader::Lexer::constructSingleConstraint(const SymbolInfo &symbol, uint16_t index,
                                                       const string &opStr, const string &valStr) {
    // Create constraint expression
    ref<Expr> left = mem[symbol.startAddr + index]->e;
    ref<Expr> right = buildConstant(processInteger(valStr));
    assert(!left.isNull() && left->getKind() == Expr::Read && "Invalid left expression retrieved");
    assert(!right.isNull() && right->getKind() == Expr::Constant && "Invalid right expression retrieved");

    ref<Expr> constraint = nullptr;

    // Only using Eq, Slt and Sle in canonical mode
    if (opStr == "==") {
        constraint = builder->Eq(left, right);
    } else if (opStr == "!=") {
        constraint = Expr::createIsZero(builder->Eq(left, right));
    } else if (opStr == "<") {
        constraint = builder->Slt(left, right);
    } else if (opStr == "<=") {
        constraint = builder->Sle(left, right);
    } else if (opStr == ">") {
        constraint = builder->Slt(right, left);
    } else if (opStr == ">=") {
        constraint = builder->Sle(right, left);
    } else {
        reportKLC3ErrExit("Unknown symbolic variable relationship \"" + opStr + "\"");
    }

    return constraint;
}

int KLC3Loader::Lexer::addSymbol(const char *symbol, int addr) {
    if (lc3Symbols.find(string(symbol)) != lc3Symbols.end()) {
        return -1;  // already exists
    }

    // This structure is for our use
    const auto &it = lc3LabelStrings.find(addr);
    if (it == lc3LabelStrings.end()) {
        lc3LabelStrings.emplace(addr, vector<string>{string(symbol)});
    } else {
        it->second.emplace_back(string(symbol));
    }

    // This structure is for lexer, which doesn't hold life cycle for label string
    lc3Symbols.emplace(string(symbol), LC3Symbol{lc3LabelStrings[addr].back().c_str(), addr});

    return 0;
}

KLC3Loader::Lexer::LC3Symbol *KLC3Loader::Lexer::findSymbol(const char *symbol) {
    const auto &it = lc3Symbols.find(string(symbol));
    if (it == lc3Symbols.end()) {
        return nullptr;  // doesn't exist
    } else {
        return &it->second;
    }
}

void KLC3Loader::Lexer::writeInst(uint16_t raw) {
    if (curPass == 2) {
        string sourceContent = curLineContent;
        InstKLC3Options options;
        unsigned long i = sourceContent.find(';');
        if (i != std::string::npos) {
            options = parseInstComment(sourceContent.substr(i + 1));
            sourceContent = sourceContent.substr(0, i);
        }
        sourceContent = strip(sourceContent);

        if (!options.sourceContent.empty()) {
            sourceContent = options.sourceContent;
        }

        ref<InstValue> val = InstValue::alloc(curAddr, buildConstant(raw));
        addMemValue(val, sourceContent);  // make changes to val
        if (loadingInputFile) {
            reportKLC3Warn("Input file " + loadingInputFile->filename + " contains instruction");
            loadingInputFile->variables.emplace_back(InputVariable{
                    val->sourceContent,
                    (uint16_t) curAddr,
                    1,
                    false,
                    false,
                    {raw},
                    nullptr
            });
        }
    }
    curAddr = (curAddr + 1) & 0xFFFF;
}

void KLC3Loader::Lexer::addMemValue(const ref<MemValue> &val, const string &sourceContent) {
    val->belongsToOS = loadingLC3OS;
    auto it = lc3LabelStrings.find(curAddr);
    if (it != lc3LabelStrings.end()) {
        val->labels = it->second;
    }
    if (UseFileBasename) {
        val->sourceFile = llvm::sys::path::filename(curFilename).str();
    } else {
        val->sourceFile = curFilename;
    }
    val->sourceLine = curLine;
    val->sourceContent = sourceContent;

    if (mem.find(curAddr) != mem.end()) {
        reportKLC3Warn("Overriding memory location " + toLC3Hex(curAddr));
    }
    mem[curAddr] = val;
}

KLC3Loader::Lexer::InstKLC3Options KLC3Loader::Lexer::parseInstComment(const string &comment) {
    InstKLC3Options ret;
    string s = strip(comment);
    if (matchStartAndTrim(s, "KLC3: ")) {
        sawKLC3Command = true;
        while (!s.empty()) {
            if (matchStartAndTrim(s, "INSTANT_HALT")) {
                ret.sourceContent = "INSTANT_HALT";
            } else {
                reportKLC3ErrExit("Unknown KLC3 command or missing argument: \"" + s + "\"");
            }
        }
    }
    return ret;
}

KLC3Loader::Lexer::DataKLC3Options KLC3Loader::Lexer::parseDataComment(const string &comment) {
    if (ignoreKLC3Extension) return {};
    DataKLC3Options ret;
    bool hasSeenRWFlags = false;
    string s = strip(comment);
    if (matchStartAndTrim(s, "KLC3: ")) {
        sawKLC3Command = true;
        while (!s.empty()) {
            if (matchStartAndTrim(s, "READ_ONLY")) {
                ret.readOnly = true;
                hasSeenRWFlags = true;
            } else if (matchStartAndTrim(s, "UNINITIALIZED")) {
                ret.uninitialized = true;
                hasSeenRWFlags = true;
            } else if (matchStartAndTrim(s, "SYMBOLIC as ")) {
                unsigned long i = s.find(' '); // can be npos
                ret.asSymbolic = s.substr(0, i);
                s = strip(s.substr(ret.asSymbolic.length()));
            } else if (matchStartAndTrim(s, "INPUT ")) {
                unsigned long i = s.find(' '); // can be npos
                ret.asInput = s.substr(0, i);
                s = strip(s.substr(ret.asInput.length()));
            } else {
                reportKLC3ErrExit("Unknown KLC3 command or missing argument: \"" + s + "\"");
            }
        }
    }
    if (!hasSeenRWFlags) {
        // Use DEFAULT
        ret.readOnly = dataDefaultReadOnly;
        ret.uninitialized = dataDefaultUninitialized;
    }
    return ret;
}

KLC3Loader::Lexer::DataKLC3Options KLC3Loader::Lexer::preProcessDataLine(string &sourceContent) {
    sourceContent = curLineContent;
    DataKLC3Options option;
    unsigned long i = sourceContent.find(';');
    if (i != std::string::npos) {
        option = parseDataComment(sourceContent.substr(i + 1));
        sourceContent = sourceContent.substr(0, i);
    }
    sourceContent = strip(sourceContent);

    if ((!option.asInput.empty() || !option.asSymbolic.empty()) && !loadingInputFile) {
        reportKLC3ErrExit("Defining input/symbolic variable in non-input file");
    }

    return option;
}

void KLC3Loader::Lexer::writeFILL(uint16_t value) {
    if (curPass == 2) {
        string sourceContent;
        DataKLC3Options options = preProcessDataLine(sourceContent);

        if (!options.asSymbolic.empty()) {
            auto &symInfo = defineMemSymbol(options.asSymbolic, options.readOnly, options.uninitialized);
            if (symInfo.size != 1) reportKLC3ErrExit("Symbolic variable on .FILL should have size 1");
            // loadingInputFile is asserted at preProcessDataLine
            loadingInputFile->variables.emplace_back(InputVariable{
                    symInfo.name,
                    symInfo.startAddr,
                    symInfo.size,
                    true,
                    false,
                    {},
                    symInfo.array
            });
            symInfo.inputVariable = &loadingInputFile->variables.back();
        } else {
            ref<DataValue> val = DataValue::alloc(curAddr, buildConstant(value),
                                                  options.readOnly, options.uninitialized);
            addMemValue(val, sourceContent);  // make changes to val
            if (loadingInputFile) {
                loadingInputFile->variables.emplace_back(InputVariable{
                        options.asInput,  // can be empty string
                        (uint16_t) curAddr,
                        1,
                        false,
                        false,
                        {value},
                        nullptr
                });
            }
        }
    }
    curAddr = (curAddr + 1) & 0xFFFF;
}

void KLC3Loader::Lexer::writeSTRINGZ(const char *o1) {
    vector<uint16_t> values;

    // We must count locations written in pass 1
    for (const char *str = o1 + 1; str[0] != '\"'; str++) {
        if (str[0] == '\\') {
            switch (str[1]) {
                // @formatter:off
                case 'a':  values.emplace_back('\a'); str++; break;
                case 'b':  values.emplace_back('\b'); str++; break;
                case 'e':  values.emplace_back('\e'); str++; break;
                case 'f':  values.emplace_back('\f'); str++; break;
                case 'n':  values.emplace_back('\n'); str++; break;
                case 'r':  values.emplace_back('\r'); str++; break;
                case 't':  values.emplace_back('\t'); str++; break;
                case 'v':  values.emplace_back('\v'); str++; break;
                case '\\': values.emplace_back('\\'); str++; break;
                case '\"': values.emplace_back('\"'); str++; break;
                default: values.emplace_back(str[1]); str++; break;
                // @formatter:on
            }
        } else {
            if (str[0] == '\n') curLine++;
            values.emplace_back(static_cast<uint16_t>((int16_t) (*str)));  // use int for signed extension
        }
    }
    values.emplace_back(0);

    if (curPass == 2) {
        string sourceContent;
        DataKLC3Options options = preProcessDataLine(sourceContent);
        if (!options.asSymbolic.empty()) {
            reportKLC3ErrExit("Do not use .STRINGZ for symbolic variable. Use .FILL or .BLKW instead.");
        } else {
            if (loadingInputFile) {
                // Do this first so that code_loc is not changed yet
                loadingInputFile->variables.emplace_back(InputVariable{
                        options.asInput,  // can be empty string
                        static_cast<uint16_t>(curAddr),
                        static_cast<uint16_t>(values.size()),
                        false,
                        true,
                        values,
                        nullptr
                });
            }
            for (auto value : values) {
                ref<DataValue> val = DataValue::alloc(curAddr, buildConstant(value),
                                                      options.readOnly, options.uninitialized);
                addMemValue(val, sourceContent);
                curAddr = (curAddr + 1) & 0xFFFF;  // addMemValue relies on code_loc, so change it each step
            }
        }
    } else {
        curAddr = (curAddr + values.size()) & 0xFFFF;
    }
}

void KLC3Loader::Lexer::writeBLKW(int count) {

    if (curPass == 2) {
        string sourceContent;
        DataKLC3Options options = preProcessDataLine(sourceContent);

        if (!options.asSymbolic.empty()) {
            auto &symInfo = defineMemSymbol(options.asSymbolic, options.readOnly, options.uninitialized);
            if (symInfo.size != count) reportKLC3ErrExit("Symbolic variable size doesn't match .BLKW size");
            // loadingInputFile is asserted at preProcessDataLine
            loadingInputFile->variables.emplace_back(InputVariable{
                    symInfo.name,
                    symInfo.startAddr,
                    symInfo.size,
                    true,
                    false,
                    {},
                    symInfo.array
            });
            symInfo.inputVariable = &loadingInputFile->variables.back();
            curAddr = (curAddr + count) & 0xFFFF;
        } else {
            if (loadingInputFile) {
                // Do this first so that code_loc is not changed yet
                loadingInputFile->variables.emplace_back(InputVariable{
                        options.asInput,  // can be empty string
                        (uint16_t) curAddr,
                        (uint16_t) count,
                        false,
                        false,
                        vector<uint16_t>(count, 0x0000),
                        nullptr
                });
            }
            for (int i = 0; i < count; i++) {
                ref<DataValue> val = DataValue::alloc(curAddr, buildConstant(0x0000),
                                                      options.readOnly, options.uninitialized);
                addMemValue(val, sourceContent);
                curAddr = (curAddr + 1) & 0xFFFF;  // addMemValue relies on code_loc, so change it each step
            }
        }
    } else {
        curAddr = (curAddr + count) & 0xFFFF;
    }
}

void KLC3Loader::Lexer::parseComment(const char *comment) {
    if (ignoreKLC3Extension) return;
    string s = strip(string(comment + 1 /* eat the ; */));
    if (matchStartAndTrim(s, "KLC3: ")) {
        sawKLC3Command = true;  // set this flag even in the first pass
        if (curPass == 1) return;  // process commands in pass 2
        while (!s.empty()) {
            if (matchStartAndTrim(s, "SET_DATA_DEFAULT_FLAG READ_ONLY")) {
                dataDefaultReadOnly = true;
            } else if (matchStartAndTrim(s, "SET_DATA_DEFAULT_FLAG UNINITIALIZED")) {
                dataDefaultUninitialized = true;
            } else if (matchStartAndTrim(s, "INPUT_FILE")) {
                if (!allowInputFile)
                    reportKLC3ErrExit("Disallow loading more INPUT_FILE for gold/test asm" + curFilename);
                inputFiles.emplace_back(InputFile{curFilename, {}, fileStartAddr, {}});
                loadingInputFile = &inputFiles.back();
            } else if (matchStartAndTrim(s, "COMMENT ")) {
                if (!loadingInputFile) reportKLC3ErrExit("COMMENT only works for INPUT_FILE");
                loadingInputFile->commentLines.emplace_back(s);  // treat all remaining string as comment
                s.clear();
            } else if (matchStartAndTrim(s, "SYMBOLIC ")) {
                processConstraintLine(s);  // treat the remaining whole line as constraint info
                s.clear();
            } else if (matchStartAndTrim(s, "ISSUE ")) {
                parseIssueCommand(s);
            } else if (matchStartAndTrim(s, "CHECK ")) {
                parsePostCheckCommand(s);
            } else {
                reportKLC3ErrExit("Unknown KLC3 command or missing argument: \"" + s + "\"");
            }
        }
    }
}

void KLC3Loader::Lexer::parseIssueCommand(string &s) {

    if (matchStartAndTrim(s, "SET_LEVEL ERR_INCORRECT_OUTPUT NONE")) {
        crossChecker->deleteDefaultOutputCompare();
        return;
    }

    if (matchStartAndTrim(s, "DEFINE ")) {

        string level, name, desc, hint;

        if (!extractNextField(s, level)) reportKLC3ErrExit("Missing level (ERROR or WARNING) for DEFINE");
        if (level != "ERROR" && level != "WARNING")
            reportKLC3ErrExit("Unknown issue level \"" + level + "\". Expecting ERROR or WARNING.");

        if (!extractNextField(s, name)) reportKLC3ErrExit("Missing issue name for DEFINE");
        if (issuePackage->getIssueByName(name) != Issue::NO_ISSUE)
            reportKLC3ErrExit("Issue named \"" + name + "\" already exists");

        if (!extractNextString(s, desc)) reportKLC3ErrExit("Missing description for DEFINE");

        if (!extractNextString(s, hint)) hint.clear();  // hint is optional

        auto issue = issuePackage->registerIssue(name, (level == "ERROR" ? Issue::ERROR : Issue::WARNING));
        progInfo() << "Register new issue " << name << " (" << level << ")\n";
        issuePackage->setIssueTypeDesc(issue, desc);
        if (!hint.empty()) issuePackage->setIssueTypeHint(issue, hint);

    } else if (matchStartAndTrim(s, "SET_LEVEL ")) {

        string name, level;

        if (!extractNextField(s, name)) reportKLC3ErrExit("Missing issue name for SET_LEVEL");
        if (!extractNextField(s, level)) reportKLC3ErrExit("Missing level (ERROR, WARNING or NONE) for SET_LEVEL");
        auto issue = issuePackage->getIssueByName(name);
        if (issue == Issue::NO_ISSUE) reportKLC3ErrExit("Unknown issue name \"" + name + "\"");
        if (level == "ERROR") {
            issuePackage->setIssueTypeLevel(issue, Issue::ERROR);
        } else if (level == "WARNING") {
            issuePackage->setIssueTypeLevel(issue, Issue::WARNING);
        } else if (level == "NONE") {
            issuePackage->setIssueTypeLevel(issue, Issue::NONE);
        } else {
            reportKLC3ErrExit("Unknown issue level \"" + level + "\". Expecting ERROR or WARNING.");
        }

    } else if (matchStartAndTrim(s, "SET_DESCRIPTION ")) {

        string name, desc;

        if (!extractNextField(s, name)) reportKLC3ErrExit("Missing issue name for SET_DESCRIPTION");
        if (!extractNextString(s, desc)) reportKLC3ErrExit("Missing description for SET_DESCRIPTION");

        auto issue = issuePackage->getIssueByName(name);
        if (issue == Issue::NO_ISSUE) reportKLC3ErrExit("Unknown issue name \"" + name + "\"");

        issuePackage->setIssueTypeDesc(issue, desc);

    } else if (matchStartAndTrim(s, "SET_HINT ")) {

        string name, hint;

        if (!extractNextField(s, name)) reportKLC3ErrExit("Missing issue name for SET_HINT");
        if (!extractNextString(s, hint)) reportKLC3ErrExit("Missing description for SET_HINT");

        auto issue = issuePackage->getIssueByName(name);
        if (issue == Issue::NO_ISSUE) reportKLC3ErrExit("Unknown issue name \"" + name + "\"");

        issuePackage->setIssueTypeHint(issue, hint);

    } else {
        this->reportKLC3ErrExit("Unknown ISSUE command or missing argument: \"" + s + "\"");
    }
}

void KLC3Loader::Lexer::parsePostCheckCommand(string &s) {

    CrossChecker::ThingToCompare thing;
    string tmp;

    // Extract type and range
    if (matchStartAndTrim(s, "OUTPUT")) {
        thing.type = CrossChecker::ThingToCompare::OUTPUT;
    } else if (matchStartAndTrim(s, "LAST_INST")) {
        thing.type = CrossChecker::ThingToCompare::LAST_INST;
    } else if (matchStartAndTrim(s, "R")) {
        thing.type = CrossChecker::ThingToCompare::REG;

        thing.from = s[0] - '0';
        s = strip(s.substr(1));
        if (!(thing.from >= 0 && thing.from <= 7)) reportKLC3ErrExit("Invalid register R" + s.substr(0, 1));
        if (matchStartAndTrim(s, "to")) {
            if (!matchStartAndTrim(s, "R")) reportKLC3ErrExit("Expecting register after \"to\"");
            thing.to = s[0] - '0';
            s = strip(s.substr(1));
            if (!(thing.to >= thing.from && thing.to <= 7))
                reportKLC3ErrExit(
                        "Invalid register R" + s.substr(0, 1) + " or smaller than R" + std::to_string(thing.from));
        } else {
            thing.to = thing.from;
        }
    } else if (matchStartAndTrim(s, "MEMORY")) {
        thing.type = CrossChecker::ThingToCompare::MEM;

        if (!extractNextField(s, tmp)) reportKLC3ErrExit("Expecting memory range");
        thing.from = processInteger(tmp);

        if (matchStartAndTrim(s, "to")) {
            if (!extractNextField(s, tmp)) reportKLC3ErrExit("Expecting memory range after \"to\"");
            thing.to = processInteger(tmp);
            if (thing.to < thing.from)
                reportKLC3ErrExit(toLC3Hex(thing.to) + " is smaller than " + toLC3Hex(thing.from));
        } else {
            thing.to = thing.from;
        }

    } else {
        reportKLC3ErrExit("Unknown CHECK item " + s);
    }

    // Extract issue
    if (!matchStartAndTrim(s, "for")) reportKLC3ErrExit("Expecting \"for <issue>\"");
    if (!extractNextField(s, tmp)) reportKLC3ErrExit("Missing issue after \"for\"");
    auto issue = issuePackage->getIssueByName(tmp);
    if (issue == Issue::NO_ISSUE) reportKLC3ErrExit("Unknown issue name \"" + tmp + "\"");

    // Extract dump gold option
    if (extractNextField(s, tmp)) {
        if (tmp == "DUMP_GOLD") {
            thing.dumpGold = true;
        } else if (tmp == "NOT_DUMP_GOLD") {
            thing.dumpGold = false;
        } else {
            reportKLC3ErrExit("Unknown option \"" + tmp + "\". Expecting DUMP_GOLD or NOT_DUMP_GOLD.");
        }
    } else {
        thing.dumpGold = false;  // by default
    }

    llvm::raw_ostream *os = nullptr;

    if (crossChecker) {
        crossChecker->registerThingToCompare(issue, thing);
        progInfo() << "Add the ";
        os = &progInfo();
    } else {
        newProgWarn() << "the ";
        os = &progWarns();
    }

    *os << "equivalence check on ";
    switch (thing.type) {
        case klc3::CrossChecker::ThingToCompare::OUTPUT:
            *os << "OUTPUT";
            break;
        case klc3::CrossChecker::ThingToCompare::REG:
            *os << "REGISTER";
            break;
        case klc3::CrossChecker::ThingToCompare::LAST_INST:
            *os << "LAST_INST";
            break;
        case klc3::CrossChecker::ThingToCompare::MEM:
            *os << "MEMORY";
            break;
    }
    *os << " for issue ";
    issuePackage->writeIssueName(*os, issue);

    if (crossChecker) {
        *os << "\n";
    } else {
        *os << " is not effective because no gold program is supplied\n";
    }
}

uint16_t KLC3Loader::Lexer::processInteger(const string &s) {
    // Exception handling is disabled in KLEE
    if (s[0] == 'x') {
        char *ptr;
        auto val = strtol(s.c_str() + 1, &ptr, 16);
        if (*ptr != '\0') reportKLC3ErrExit("Invalid integer " + s);
        return val;
    } else if (s[0] == '#') {
        char *ptr;
        auto val = strtol(s.c_str() + 1, &ptr, 10);
        if (*ptr != '\0') reportKLC3ErrExit("Invalid integer " + s);
        return val;
    } else {
        reportKLC3ErrExit("Invalid integer \"" + s + "\". Please use LC3 syntax (#1234 or xABCD).");
        return 0;
    }
}

bool KLC3Loader::Lexer::matchStartAndTrim(string &s, const string &prefix) {
    if (startswith(s, prefix)) {
        s = strip(s.substr(prefix.length()));
        return true;
    } else {
        return false;
    }
}

bool KLC3Loader::Lexer::extractNextField(string &s, string &field) {
    if (s.empty()) return false;
    auto pos = s.find(' ');
    field = s.substr(0, pos);  // pos can be npos
    s = strip(s.substr(field.length()));
    return true;
}

bool KLC3Loader::Lexer::extractNextString(string &s, string &str) {
    if (s[0] != '"') return false;
    s = s.substr(1);
    auto pos = s.find('"');
    if (pos == string::npos) return false;
    str = s.substr(0, pos);
    s = strip(s.substr(pos + 1));
    return true;
}

void KLC3Loader::Lexer::reportKLC3ErrExit(const llvm::Twine &error) const {
    newProgErr() << error << "\n"
                 << "  Occur at: " << "[" << curFilename << ":" << curLine << "] " << curLineContent << "\n";
    progExit();
}

void KLC3Loader::Lexer::reportKLC3Warn(const llvm::Twine &warn) const {
    newProgWarn() << warn << "\n"
                  << "  Occur at: " << "[" << curFilename << ":" << curLine << "] " << curLineContent << "\n";
}

void KLC3Loader::Lexer::reportCompileErr(const llvm::Twine &error) {
    if (ignoreKLC3Extension || !collectCompileIssues) {
        fprintf(stderr, "%3d: %s\n", curLine, error.str().c_str());
    } else {
        raiseCompileIssue(true, error);
    }
}

void KLC3Loader::Lexer::reportCompileWarn(const llvm::Twine &warn) {
    if (ignoreKLC3Extension || !collectCompileIssues) {
        fprintf(stderr, "%3d: WARNING: %s\n", curLine, warn.str().c_str());
    } else {
        raiseCompileIssue(false, warn);
    }
}

void KLC3Loader::Lexer::raiseCompileIssue(bool isErr, const llvm::Twine &note) {
    auto val = ref<MemValue>(new MemValue(0, nullptr));
    if (UseFileBasename) {
        val->sourceFile = llvm::sys::path::filename(curFilename).str();
    } else {
        val->sourceFile = curFilename;
    }
    val->sourceLine = curLine;
    val->sourceContent = curLineContent;
    auto info = issuePackage->newGlobalIssue((isErr ? Issue::ERR_COMPILE : Issue::WARN_COMPILE), val);
    info->setNote(note);
}

}

