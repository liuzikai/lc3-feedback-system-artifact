//
// Created by liuzikai on 5/23/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_RESULTGENERATOR_H
#define KLC3_RESULTGENERATOR_H

#include "klc3/Core/State.h"
#include "klc3/Loader/InputVariable.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

namespace klc3 {

extern llvm::cl::OptionCategory KLC3OutputCat;

class ResultGenerator {
public:

    /**
     * Create an instance of ResultGenerator
     * @param outputPath      Directory to store output file. The directory will be created if not exists. If an empty
     *                        string is given, you can still use methods provided by this class, but no files will be
     *                        actually generated.
     * @param loadedPrograms  List of loaded asm files (excluding inputFiles)
     * @param inputFiles
     * @param initPC
     */
    ResultGenerator(PathString outputPath, const vector<string> &loadedPrograms, vector <InputFile> inputFiles, uint16_t initPC);

    /**
     * Complete a state and generate its package
     * @param state
     * @param assignment
     * @param stepIntervals
     * @return Test case basename
     */
    string completeState(State *state, const Assignment &assignment, const vector<int> &stepIntervals);

    /**
     * Generate the global report
     * @param issuePackage
     */
    void generateGlobalReport(const IssuePackage &issuePackage) const;

private:

    typedef int TestCaseIndex;
    TestCaseIndex generatedCaseCount;

    TestCaseIndex getNewTestCaseIndex();

    static string getTestCaseBaseName(const State *s, TestCaseIndex index) ;

    map<const State *, string> testCaseNames;

    PathString outputPath;

    string postExecutionScript;

    vector <InputFile> inputFiles;

    uint16_t initPC;

    vector<string> filesToLoad;

    // Subroutines and their helper functions

    /**
     * Generate init memory asm(s), entry point asm and kquery file if needed for a test case
     * @param s
     * @param baseName
     * @param assignment
     * @param stepIntervals
     */
    void generateTestCase(State *s, const string &baseName, const Assignment &assignment, const vector<int> &stepIntervals) const;

    /**
     * Generate one or more asm files for mem-type symbolic values
     * @param s
     * @param baseName
     * @param assignment
     * @param generatedFiles [out] list of generated files, APPEND
     */
    void generateMemInitValueASM(State *s, const string &baseName, const Assignment &assignment, vector<string> &generatedFiles) const;

    /**
     * Generate lc3sim script file for a test case
     * @param s
     * @param fileName
     * @param files
     * @param testInitPC
     * @param stepIntervals
     */
    void generateLC3SimScript(const State *s, StringRef fileName, const vector<string> &files,
                              uint16_t testInitPC, const vector<int> &stepIntervals) const;

    static void generateLC3OutToStream(llvm::raw_ostream &out, State *s);

    std::unique_ptr<llvm::raw_fd_ostream> openOutputFile(StringRef fileName) const;

};

}

#endif //KLEE_RESULTGENERATOR_H
