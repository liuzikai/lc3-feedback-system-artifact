//
// Created by liuzikai on 5/23/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Generation/ResultGenerator.h"
#include "klc3/Generation/ReportFormatter.h"
#include "klee/Expr/ExprPPrinter.h"
#include "llvm/Support/CommandLine.h"

#include <memory>
#include <utility>
#include <fstream>

namespace klc3 {

llvm::cl::opt<string> ReportBasename(
        "report-basename",
        llvm::cl::desc(
                "Basename for report file, excluding extension (determined by report-style option) (default=\"report\""),
        llvm::cl::init("report"),
        llvm::cl::cat(KLC3ReportCat));

llvm::cl::OptionCategory KLC3OutputCat("KLC3 output options");

llvm::cl::opt<bool> GenerateAdvancedBreakpoints(
        "generate-advanced-breakpoints",
        llvm::cl::desc(
                "Generate advanced breakpoints in lc3sim script (default=false)"),
        llvm::cl::init(false)
        // ,llvm::cl::cat(KLC3OutputCat)
        // Hidden for now
);

llvm::cl::opt<bool> SubdirectoryStates(
        "subdirectory-states",
        llvm::cl::desc("Put files of each test cases into subdirectories (default=true)"),
        llvm::cl::init(true),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::opt<bool> OutputForReplay(
        "output-for-replay",
        llvm::cl::desc("Copy necessary files preparing for replay (default=true)"),
        llvm::cl::init(true),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::opt<bool> OutputReportToTerminal(
        "report-to-terminal",
        llvm::cl::desc("Output report to terminal (default=true)"),
        llvm::cl::init(true),
        llvm::cl::cat(KLC3DebugCat));

llvm::cl::opt<bool> OutputLC3OutToTerminal(
        "lc3-out-to-terminal",
        llvm::cl::desc("Output lc3 out of each state to terminal (default=false)"),
        llvm::cl::init(false),
        llvm::cl::cat(KLC3DebugCat));

llvm::cl::opt<string> PostExecutionScriptFilename(
        "post-exec-script",
        llvm::cl::desc("A file containing lc3sim script commands that will be appended to the end of generated "
                       "lc3sim scripts. Empty string to disable (default)."),
        llvm::cl::init(""),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::opt<string> TestCaseNameSuffix(
        "test-case-name-suffix",
        llvm::cl::desc("Suffix for test case names (default=\"\")"),
        llvm::cl::init(""),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::opt<int> TestCaseStartIndex(
        "test-case-start-index",
        llvm::cl::desc("The starting index of the generated test cases (default=0)"),
        llvm::cl::init(0),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::opt<string> UniqueTestCaseName(
        "unique-test-case-name",
        llvm::cl::desc("For test with no symbolic variable (one and only one state), "
                       "use the given name if the test case is to be generate. "
                       "Error will be raised if trying to generate more than one state. "
                       "Empty string to disable (default)."),
        llvm::cl::init(""),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::list<string> AdditionalFiles(
        "copy-additional-file",
        llvm::cl::desc("Copy an additional file to the output directory, such as "
                       "the replay script. Multiple such options are allow, one for a file."),
        llvm::cl::value_desc("file"),
        llvm::cl::ZeroOrMore,
        llvm::cl::cat(KLC3OutputCat));

ResultGenerator::ResultGenerator(PathString outputPath_, const vector<string> &loadedPrograms,
                                 vector<InputFile> inputFiles_, uint16_t initPC)
        : generatedCaseCount(TestCaseStartIndex), outputPath(std::move(outputPath_)),
          inputFiles(std::move(inputFiles_)), initPC(initPC) {

    for (const auto &filename: loadedPrograms) {
        const StringRef &basename = llvm::sys::path::filename(filename);  // file at local copy
        if (!outputPath.empty() && OutputForReplay) {
            if (llvm::StringRef(filename).endswith_lower("_.asm")) continue;  // do not copy file ends with "_"
            if (llvm::sys::fs::exists(filename)) {
                PathString destFilename(outputPath);
                llvm::sys::path::append(destFilename, basename);
                llvm::sys::fs::copy_file(filename, destFilename);
            } else {
                newProgErr() << "File " << filename << " should exists" << "\n";
                progExit();
            }
        }
        filesToLoad.emplace_back(basename.substr(0, basename.size() - 4));  // remove extension
        // The order of loading asm files doesn't matter since we are going to set PC manually
    }

    if (!outputPath.empty() && OutputForReplay) {
        // Copy additional file
        for (const auto &file : AdditionalFiles) {
            if (llvm::sys::fs::exists(file)) {
                PathString destFilename(outputPath);
                llvm::sys::path::append(destFilename, llvm::sys::path::filename(file));
                llvm::sys::fs::copy_file(file, destFilename);
            } else {
                newProgErr() << "Additional file " << file << " doesn't exists" << "\n";
                progExit();
            }
        }
    }

    if (!PostExecutionScriptFilename.empty()) {
        std::ifstream postScriptFile(PostExecutionScriptFilename, std::ios::in | std::ios::binary);
        if (!postScriptFile) {
            newProgErr() << "Failed to open file " << PostExecutionScriptFilename << "\n";
            progExit();
        }
        postExecutionScript.assign(std::istreambuf_iterator<char>(postScriptFile), std::istreambuf_iterator<char>());
    }
}

void ResultGenerator::generateGlobalReport(const IssuePackage &issuePackage) const {
    if (OutputReportToTerminal) {
        llvm::outs() << "================================ REPORT ================================\n";
        ReportFormatterBrief().generateReport(llvm::outs(), issuePackage);
        llvm::outs() << "============================ END OF REPORT =============================\n";
    }

    if (!outputPath.empty()) {
        auto outPtr = openOutputFile(ReportBasename + reportFormatter->getReportExtension());
        reportFormatter->generateReport(*outPtr, issuePackage);
    }
}

string ResultGenerator::completeState(State *state, const Assignment &assignment, const vector<int> &stepIntervals) {

    TestCaseIndex index = getNewTestCaseIndex();
    string baseName = getTestCaseBaseName(state, index);
    testCaseNames[state] = baseName;

    if (!outputPath.empty() && OutputForReplay) {
        if (SubdirectoryStates) {
            // Use basename itself as subdirectory
            PathString subdir(outputPath);
            llvm::sys::path::append(subdir, baseName);
            llvm::sys::fs::create_directory(subdir);
            baseName = baseName + "/" + baseName;  // use relative path
        }
        generateTestCase(state, baseName, assignment, stepIntervals);
    }

    if (OutputLC3OutToTerminal) {
        llvm::outs() << "================ TEST CASE " << index << " OUT ================\n";
        generateLC3OutToStream(llvm::outs(), state);
        llvm::outs() << "================ END OF TEST CASE " << index << " OUT ================\n\n";
    }

    return testCaseNames[state];
}

void ResultGenerator::generateTestCase(State *s, const string &baseName, const Assignment &assignment,
                                       const vector<int> &stepIntervals) const {
    vector<string> generatedFiles = filesToLoad;  // should be .obj file or no extension, should not be .asm file
    // The order of generatedASMFiles is is the order of loading by lc3sim

    // Output memory init value asm, should be latter than original asm to properly overwrite symbolic values
    generateMemInitValueASM(s, baseName, assignment, generatedFiles);

    // LC3Sim script file
    generateLC3SimScript(s, baseName + ".lcs", generatedFiles, initPC, stepIntervals);
    // Notice that the script file is not loading .asm file. Use basename instead
}

void ResultGenerator::generateMemInitValueASM(State *s, const string &baseName, const Assignment &assignment,
                                              vector<string> &generatedFiles) const {
    (void) s;

    for (const auto &inputFile : inputFiles) {

        string filename = baseName + "-" + llvm::sys::path::filename(inputFile.filename).str();
        generatedFiles.emplace_back(filename.substr(0, filename.length() - 4));  // remove .asm

        // Header and comments
        auto outPtr = openOutputFile(filename);
        (*outPtr) << "; Test case generated by KLC3\n\n";
        if (!inputFile.commentLines.empty()) {
            for (const auto &line : inputFile.commentLines) {
                (*outPtr) << "; " << line << "\n";
            }
            (*outPtr) << "\n";
        }

        // .ORIG
        (*outPtr) << ".ORIG " << toLC3Hex(inputFile.startAddr) << "\n\n";

        // Input variables
        uint16_t curAddr = inputFile.startAddr;
        for (const auto &var : inputFile.variables) {
            assert(curAddr == var.startAddr && "Missing inputVariable in an inputFile");

            // Get values
            vector<uint16_t> values;
            if (var.isSymbolic) {
                values.reserve(var.size);
                assert(var.array && var.array->size == var.size);
                for (unsigned i = 0; i < var.array->size; i++) {
                    ref<Expr> realizedValue = assignment.evaluate(var.array, i);
                    assert(realizedValue->getWidth() == Expr::Int16 &&
                           "Invalid size of init values. "
                           "Make sure you are using the solver that is revised to support Int16 range");

                    values.emplace_back(castConstant(realizedValue));
                }
            } else {
                values = var.fixedValues;
            }
            assert(values.size() == var.size);

            // Generate asm
            if (var.isString) {
                // Single .STRINGZ, put name on the same line, .FILL for remaining bits, one blank line after
                unsigned i = 0;
                (*outPtr) << ".STRINGZ \"";
                while (i < values.size() /* for safety */ && values[i] != 0) {
                    switch (values[i]) {
                        // @formatter:off
                        case '\a':  (*outPtr) << "\\a";  break;
                        case '\b':  (*outPtr) << "\\b";  break;
                        case '\e':  (*outPtr) << "\\e";  break;
                        case '\f':  (*outPtr) << "\\f";  break;
                        case '\n':  (*outPtr) << "\\n";  break;
                        case '\r':  (*outPtr) << "\\r";  break;
                        case '\t':  (*outPtr) << "\\t";  break;
                        case '\v':  (*outPtr) << "\\v";  break;
                        case '\\':  (*outPtr) << "\\\\"; break;
                        case '\"':  (*outPtr) << "\\\""; break;
                        default:    (*outPtr) << static_cast<char>(values[i]); break;
                        // @formatter:on
                    }
                    i++;
                }
                assert(i < values.size() && values[i] == 0 && "String variable don't have NULL terminal");
                i++;  // eat up the NULL
                (*outPtr) << "\"";
                if (!var.name.empty()) (*outPtr) << "\t; " << var.name;
                (*outPtr) << "\n";

                if (i < values.size()) {  // still have remaining bits

                    // The first one, with comment
                    (*outPtr) << ".FILL " << toLC3Hex(values[i++]);
                    if (!var.name.empty()) (*outPtr) << "\t; bits after " << var.name;
                    (*outPtr) << "\n";

                    // The remaining without comment
                    while (i < values.size()) {
                        (*outPtr) << ".FILL " << toLC3Hex(values[i++]) << "\n";
                    }
                }

                (*outPtr) << "\n";  // blank line

            } else {

                if (var.size == 1) {

                    // Single .FILL, put name on the same line, one blank line after
                    (*outPtr) << ".FILL " << toLC3Hex(values[0]);
                    if (!var.name.empty()) (*outPtr) << "\t; " << var.name;
                    (*outPtr) << "\n\n";

                } else {

                    // Name and size as comment on the first line, .FILL, one blank line after
                    if (!var.name.empty()) (*outPtr) << "; " << var.name << "[" << var.size << "]\n";
                    for (auto val : values) {
                        (*outPtr) << ".FILL " << toLC3Hex(val) << "\n";
                    }
                    (*outPtr) << "\n";  // blank line
                }
            }

            curAddr += var.size;
        }

        // .END
        (*outPtr) << ".END\n";
        outPtr->close();
    }
}

void ResultGenerator::generateLC3SimScript(const State *s, StringRef fileName, const vector<string> &files,
                                           uint16_t testInitPC, const vector<int> &stepIntervals) const {
    (void) s;
    auto outPtr = openOutputFile(fileName);
    if (!outPtr) return;
    auto &out = *outPtr;

    // Load files, test case files should be place latter
    for (const auto &f : files) {
        out << "f " << f << "\n";
    }

    // Set PC
    out << "reg PC " << toLC3Hex(testInitPC) << "\n";

    if (GenerateAdvancedBreakpoints) {
        for (int interval : stepIntervals) {
            out << "step " << interval << "\n";
        }
    }

    // Continue
    // out << "continue" << "\n";

    if (!postExecutionScript.empty()) {
        out << postExecutionScript << "\n";
    }

    out.close();
}

void ResultGenerator::generateLC3OutToStream(llvm::raw_ostream &out, State *s) {
    for (const auto &value : s->lc3Out) {
        if (value->getKind() == Expr::Constant) {
            auto *ce = dyn_cast<ConstantExpr>(value);
            /*
             * In lc3sim, memory write to DDR results in formatted print of %c with an int supplied.
             * We simulate the same behavior here.
             */
            out << static_cast<char>(ce->getZExtValue(Expr::Int16));
        } else {
            vector<pair<uint16_t, ref<Expr>>> values;
            out << "{Symbolic}";
        }
    }
}

ResultGenerator::TestCaseIndex ResultGenerator::getNewTestCaseIndex() {
    return generatedCaseCount++;
}

string ResultGenerator::getTestCaseBaseName(const State *s, TestCaseIndex index) {
    (void) s;  // not use for now
    if (UniqueTestCaseName.empty()) {
        std::stringstream ret;
        ret << "test" << index << TestCaseNameSuffix;
        return ret.str();
    } else {
        assert(index == 0 && "Generate more than one state while unique-test-case-name is given");
        return UniqueTestCaseName;
    }
}

std::unique_ptr<llvm::raw_fd_ostream> ResultGenerator::openOutputFile(StringRef fileName) const {
    PathString name(outputPath);
    llvm::sys::path::append(name, fileName);
    std::error_code errorCode;
    auto ret = std::make_unique<llvm::raw_fd_ostream>(name, errorCode, llvm::sys::fs::F_None);
    if (errorCode) {
        newProgErr() << "Failed to open " << fileName << "\n";
        progExit();
        return nullptr;  // just to avoid warning
    } else {
        return ret;
    }
}

}