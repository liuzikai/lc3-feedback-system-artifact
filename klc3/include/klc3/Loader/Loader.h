//
// Created by liuzikai on 3/19/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_LOADER_H
#define KLC3_LOADER_H

#include "klc3/Common.h"
#include "klc3/Core/MemoryValue.h"
#include "klc3/Loader/InputVariable.h"
#include "klee/Expr/ArrayCache.h"

#if !defined(yyFlexLexerOnce)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"

#include <FlexLexer.h>

#pragma clang diagnostic pop
#endif

namespace klc3 {

class IssuePackage;

class CrossChecker;

using klee::ArrayCache;

class KLC3Loader : protected WithBuilder {
public:

    explicit KLC3Loader(ExprBuilder *builder, ArrayCache *arrayCache, IssuePackage *issuePackage,
                        CrossChecker *crossChecker, bool ignoreKLC3Extension = false);

    KLC3Loader(const KLC3Loader &loader) = default;

    void loadLC3OS(const string &filename);

    /**
     * Load an asm file.
     * @param filename
     * @param isLC3OS
     * @param allowInputFile        If true, KLC3 command INPUT_FILE is allowed (can define symbolic variables).
     *                              Should be true for shared input files but not for gold and test programs.
     * @param collectCompileIssues  If true, compile errors and warnings are collected into the IssuePackage.
     *                              Otherwise, errors and warnings are printed to stderr and program exits.
     * @return If collectCompileIssues, true if there is no compile errors (not including warnings).
     */
    bool load(const string &filename, bool isLC3OS, bool allowInputFile, bool collectCompileIssues);

    const map<uint16_t, ref<MemValue>> &getMem() const { return mem; }

    const vector<ref<Expr>> &getConstraints() const { return constraints; }

    const vector<ref<Expr>> &getPreferences() const { return preferences; }

    uint16_t getInitPC() const { return initPC; }

    const vector<string> &getLoadedPrograms() const { return loadedPrograms; }  // excluding inputFiles

    const vector<InputFile> &getInputFiles() const { return inputFiles; }

private:

    ArrayCache *arrayCache;

    IssuePackage *issuePackage;
    CrossChecker *crossChecker;

    struct SymbolInfo {
        string name;
        uint16_t size;
        uint16_t startAddr;
        const Array *array;
        InputVariable *inputVariable = nullptr;
    };

    unordered_map<string, SymbolInfo> symbols;

    map<uint16_t, ref<MemValue>> mem;

    vector<ref<Expr>> constraints;
    vector<ref<Expr>> preferences;

    uint16_t initPC;

    vector<string> loadedPrograms;  // excluding inputFiles
    vector<InputFile> inputFiles;

    const bool ignoreKLC3Extension;

    class Lexer : protected WithBuilder, protected yyFlexLexer {
    public:

        Lexer(ExprBuilder *builder, ArrayCache *arrayCache, IssuePackage *issuePackage, CrossChecker *crossChecker,
              unordered_map<string, SymbolInfo> &symbols, map<uint16_t, ref<MemValue>> &mem,
              vector<ref<Expr>> &constraints, vector<ref<Expr>> &preferences, uint16_t &initPC,
              vector<string> &loadedPrograms, vector<InputFile> &inputFiles, bool allowInputFile,
              bool ignoreKLC3Extension = false)
                : WithBuilder(builder), arrayCache(arrayCache), issuePackage(issuePackage), crossChecker(crossChecker),
                  symbols(symbols), mem(mem), constraints(constraints), preferences(preferences), initPC(initPC),
                  loadedPrograms(loadedPrograms), inputFiles(inputFiles), allowInputFile(allowInputFile),
                  ignoreKLC3Extension(ignoreKLC3Extension) {}

        bool load(const string &filename, bool isLC3OS, bool collectCompileIssues);

    private:

        // Reference to parent Loader instance
        ArrayCache *arrayCache;
        IssuePackage *issuePackage;
        CrossChecker *crossChecker;
        unordered_map<string, SymbolInfo> &symbols;
        map<uint16_t, ref<MemValue>> &mem;
        vector<ref<Expr>> &constraints;
        vector<ref<Expr>> &preferences;
        uint16_t &initPC;
        vector<string> &loadedPrograms;  // excluding inputFiles
        vector<InputFile> &inputFiles;
        const bool allowInputFile;
        const bool ignoreKLC3Extension;
        bool collectCompileIssues;

        enum OPCode {
            /* no opcode seen (yet) */
            OP_NONE,

            /* real instruction opcodes */
            OP_ADD, OP_AND, OP_BR, OP_JMP, OP_JSR, OP_JSRR, OP_LD, OP_LDI, OP_LDR,
            OP_LEA, OP_NOT, OP_RTI, OP_ST, OP_STI, OP_STR, OP_TRAP,

            /* trap pseudo-ops */
            OP_GETC, OP_HALT, OP_IN, OP_OUT, OP_PUTS, OP_PUTSP,

            /* non-trap pseudo-ops */
            OP_FILL, OP_RET, OP_STRINGZ,

            /* directives */
            OP_BLKW, OP_END, OP_ORIG,

            NUM_OPS
        };

        static constexpr const char *OPNAMES[NUM_OPS] = {
                /* no opcode seen (yet) */
                "missing opcode",

                /* real instruction opcodes */
                "ADD", "AND", "BR", "JMP", "JSR", "JSRR", "LD", "LDI", "LDR", "LEA",
                "NOT", "RTI", "ST", "STI", "STR", "TRAP",

                /* trap pseudo-ops */
                "GETC", "HALT", "IN", "OUT", "PUTS", "PUTSP",

                /* non-trap pseudo-ops */
                ".FILL", "RET", ".STRINGZ",

                /* directives */
                ".BLKW", ".END", ".ORIG",
        };

        enum CCode {
            CC_ = 0,
            CC_P = 0x0200,
            CC_Z = 0x0400,
            CC_N = 0x0800
        };

        enum OperandType {
            O_RRR, O_RRI,
            O_RR, O_RI, O_RL,
            O_R, O_I, O_L, O_S,
            O_,
            NUM_OPERANDS
        };

        struct Inst {
            OPCode op;
            int ccode;
        };

        static constexpr int OP_FORMAT_OK[NUM_OPS] = {
                /* no opcode seen (yet) */
                0x200, /* no opcode, no operands       */

                /* real instruction formats */
                0x003, /* ADD: RRR or RRI formats only */
                0x003, /* AND: RRR or RRI formats only */
                0x0C0, /* BR: I or L formats only      */
                0x020, /* JMP: R format only           */
                0x0C0, /* JSR: I or L formats only     */
                0x020, /* JSRR: R format only          */
                0x018, /* LD: RI or RL formats only    */
                0x018, /* LDI: RI or RL formats only   */
                0x002, /* LDR: RRI format only         */
                0x018, /* LEA: RI or RL formats only   */
                0x004, /* NOT: RR format only          */
                0x200, /* RTI: no operands allowed     */
                0x018, /* ST: RI or RL formats only    */
                0x018, /* STI: RI or RL formats only   */
                0x002, /* STR: RRI format only         */
                0x040, /* TRAP: I format only          */

                /* trap pseudo-op formats (no operands) */
                0x200, /* GETC: no operands allowed    */
                0x200, /* HALT: no operands allowed    */
                0x200, /* IN: no operands allowed      */
                0x200, /* OUT: no operands allowed     */
                0x200, /* PUTS: no operands allowed    */
                0x200, /* PUTSP: no operands allowed   */

                /* non-trap pseudo-op formats */
                0x0C0, /* .FILL: I or L formats only   */
                0x200, /* RET: no operands allowed     */
                0x100, /* .STRINGZ: S format only      */

                /* directive formats */
                0x040, /* .BLKW: I format only         */
                0x200, /* .END: no operands allowed    */
                0x040  /* .ORIG: I format only         */
        };

        enum PreParse {
            NO_PP = 0,
            PP_R1 = 1,
            PP_R2 = 2,
            PP_R3 = 4,
            PP_I2 = 8,
            PP_L2 = 16
        };

        static constexpr unsigned PRE_PARSE[NUM_OPERANDS] = {
                (PP_R1 | PP_R2 | PP_R3), /* O_RRR */
                (PP_R1 | PP_R2),         /* O_RRI */
                (PP_R1 | PP_R2),         /* O_RR  */
                (PP_R1 | PP_I2),         /* O_RI  */
                (PP_R1 | PP_L2),         /* O_RL  */
                PP_R1,                   /* O_R   */
                NO_PP,                   /* O_I   */
                NO_PP,                   /* O_L   */
                NO_PP,                   /* O_S   */
                NO_PP                    /* O_    */
        };


        string curFilename;
        bool loadingLC3OS = false;
        bool dataDefaultReadOnly = false;
        bool dataDefaultUninitialized = false;
        InputFile *loadingInputFile = nullptr;  // created and set if detecting INPUT_FILE command

        struct StringCaseCmp {  // case insensitive compare
            bool operator()(const string &s1, const string &s2) const {
                return strcasecmp(s1.c_str(), s2.c_str()) < 0;
            }
        };

        struct LC3Symbol {
            const char *name;
            int addr;
        };
        map<string, LC3Symbol, StringCaseCmp> lc3Symbols;  // for lexer

        int addSymbol(const char *symbol, int addr);  // 0 for success, -1 for duplication
        LC3Symbol *findSymbol(const char *symbol);

        map<uint16_t, vector<string>> lc3LabelStrings;  // for generation of MemValue

        string curLineContent;

        int curPass, curLine, errCount, sawORIG, sawEND, sawKLC3Command, curAddr;
        uint16_t fileStartAddr;
        Inst inst;

        void newInstLine();

        void appendLineContent(const char *opname);

        void badOperands();

        void unterminatedString();

        void badLine();

        void lineIgnored();

        void parseCCode(const char *);

        int findLabel(const char *optarg, int bits);

        int readVal(const char *s, int *vptr, int bits);

        void generateInstruction(OperandType, const char *);

        void foundLabel(const char *lname);

        int yylex();  // in Lexer.f
        void resetLexer();  // in Lexer.f

        struct InstKLC3Options {
            string sourceContent;
        };

        /**
         * Parse comment after an instruction
         * @param comment comment string without ';'
         * @return
         */
        InstKLC3Options parseInstComment(const string &comment);

        void writeInst(uint16_t raw);

        struct DataKLC3Options {
            bool readOnly = false;
            bool uninitialized = false;
            string asInput;
            string asSymbolic;
        };

        /**
         * Parse comment after a data line
         * @param comment comment string without ';'
         * @return
         */
        DataKLC3Options parseDataComment(const string &comment);

        /**
         * Preprocess a data line
         * @param sourceContent [out] source content
         * @return
         */
        DataKLC3Options preProcessDataLine(string &sourceContent);

        void writeFILL(uint16_t value);

        void writeSTRINGZ(const char *str);

        void writeBLKW(int count);

        void addMemValue(const ref<MemValue> &val, const string &sourceContent);

        void parseComment(const char *comment);

        void parseIssueCommand(string &s);

        void parsePostCheckCommand(string &s);

        // ================================ Symbols and Constraints ================================

        KLC3Loader::SymbolInfo &defineMemSymbol(string nameAndSize, bool forRead, bool forWrite);

        void processConstraintLine(const string &line);

        void processDefiningConstraint(const vector<string> &fields);

        void addConstraintOfString(const SymbolInfo &symbol, bool variableLength);

        void processNumericalConstraint(const string &line);

        ref<Expr> processSingleConstraint(const vector<string> &fields);

        ref<Expr> constructSingleConstraint(const SymbolInfo &symbol, uint16_t index,
                                            const string &opStr, const string &valStr);

        // Increment the lower bound for character preference so that it's easier for students to differ them
        int charPreferenceLowerBound = 0x41;  // starting from A

        // ================================ String Processing Helpers ================================

        static bool matchStartAndTrim(string &s, const string &prefix);

        static bool extractNextField(string &s, string &field);

        static bool extractNextString(string &s, string &str);

        static void splitNameAndSubscript(const string &s, string &name, uint16_t &size);

        static void splitString(const string &str, vector<std::string> &result, const char delimiter);

        uint16_t processInteger(const string &s);

        void reportKLC3ErrExit(const llvm::Twine &error) const;

        void reportKLC3Warn(const llvm::Twine &warn) const;

        void reportCompileErr(const llvm::Twine &error);

        void reportCompileWarn(const llvm::Twine &warn);

        void raiseCompileIssue(bool isErr, const llvm::Twine &note);
    };
};

}


#endif //KLEE_LOADER_H
