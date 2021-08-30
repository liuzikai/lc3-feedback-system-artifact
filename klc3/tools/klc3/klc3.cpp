//
// Created by liuzikai on 3/20/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Common.h"
#include "klc3/Core/Executor.h"
#include "klc3/Loader/Loader.h"
#include "klc3/FlowAnalysis/FlowGraph.h"
#include "klc3/FlowAnalysis/SubroutineTracker.h"
#include "klc3/FlowAnalysis/LoopAnalyzer.h"
#include "klc3/FlowAnalysis/FlowGraphVisualizer.h"
#include "klc3/Generation/ReportFormatter.h"
#include "klc3/Verification/CrossChecker.h"
#include "klc3/Verification/ExecutionLimitChecker.h"
#include "klc3/Generation/IssueFilter.h"
#include "klc3/Generation/ResultGenerator.h"
#include "klc3/Generation/VariableInductor.h"
#include "klc3/Searcher/Searcher.h"
#include "klc3/Searcher/PruningSearcher.h"

#include "klee/Support/OptionCategories.h"
#include "klee/Solver/Solver.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <signal.h>

#include "llvm/Support/Signals.h"

#define ENABLE_STATE_EARLY_RELEASE          1
#define ENABLE_GOLD_LOOP_COVERAGE           0
#define ENABLE_ISSUE_FILTER                 1

#define ALARM_INTERVAL                      1     // [s], handle report/timeout per this time using SIGALRM
#define INTERACTIVE_ECHO                    0

#define DUMP_LOOPS_TO_TERMINAL              1
#define DUMP_LOOPS_TO_FILE                  1  // dump loop information to loops.log
#define VISUALIZE_LOOPS                     1  // dump loop png or dot (depends on GENERATE_DOT_INSTEAD_OF_IMAGE) to loops/

#define VISUALIZE_SEGMENT_COVERING_PATHS    0
#define DUMP_SEGMENT_COVERING_STATES        0

#define MAXIMAL_SEGMENT_COUNT               1000

// Dump to file options only work with not "none" OutputDirectory

using namespace klc3;

namespace klc3 {

llvm::cl::opt<string> OutputDirectory(
        "output-dir",
        llvm::cl::desc("Output directory that contains report and generated test cases.\n"
                       "If not specified, klc3 will output to 'klc3-out-*' directory.\n"
                       "If set as \"none\", no output files will be created (dry-run mode).\n"
                       "Otherwise, output will be written to the given directory."),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::opt<string> MaxTimeRaw(
        "max-time",
        llvm::cl::desc("Stop exploring the test program and generate result after this amount of time "
                       "(0 for no constraint, default)"),
        llvm::cl::init("0"),
        llvm::cl::cat(KLC3ExecutionCat));

klee::time::Span MaxTime;

llvm::cl::opt<string> EarlyExitTimeRaw(
        "early-exit-time",
        llvm::cl::desc("If this amount of time has passed and at least one issue is detected, "
                       "stop exploring and generate result"
                       " (0 for no constraint, default)"),
        llvm::cl::init("0"),
        llvm::cl::cat(KLC3ExecutionCat));

klee::time::Span EarlyExitTime;

enum class SearcherOption {
    SimpleFILO,
    PrioritizedFILO,
    Pruning
};

llvm::cl::opt<SearcherOption> SearcherType(
        "searcher",
        llvm::cl::desc("Search heuristic to use (default=prioritized_filo)"),
        llvm::cl::values(
                clEnumValN(SearcherOption::SimpleFILO, "simple_filo", "SimpleFILOSearcher"),
                clEnumValN(SearcherOption::PrioritizedFILO, "prioritized_filo", "PrioritizedFILOSearcher"),
                clEnumValN(SearcherOption::Pruning, "pruning", "PruningSearcher")
        ),
        llvm::cl::init(SearcherOption::PrioritizedFILO),
        llvm::cl::cat(KLC3ExecutionCat));

enum class ReactivationOption {
    None,
    Stop
};

llvm::cl::opt<ReactivationOption> ReactivationOperation(
        "pruning-reactivation",
        llvm::cl::desc("Operation at reactivation (valid only when -searcher=pruning, default=none)"),
        llvm::cl::values(
                clEnumValN(ReactivationOption::None, "none", "Do nothing. Continue for reactivation."),
                clEnumValN(ReactivationOption::Stop, "stop", "Stop and report founded issues.")
        ),
        llvm::cl::init(ReactivationOption::None),
        llvm::cl::cat(KLC3ExecutionCat));

llvm::cl::opt<bool> GenerateFinalFlowGraph(
        "output-flowgraph",
        llvm::cl::desc("Generate final flow graph on edge coverage (default=true)"),
        llvm::cl::init(true),
        llvm::cl::cat(KLC3OutputCat));

llvm::cl::OptionCategory KLC3InputCat("KLC3 input");

llvm::cl::list<string> InputFiles(llvm::cl::Positional,
                                  llvm::cl::desc("<program/shared program>..."),
                                  llvm::cl::ZeroOrMore,
                                  llvm::cl::cat(KLC3InputCat));

llvm::cl::list<string> TestPrograms("test",
                                    llvm::cl::desc("Test asm file. Can have multiple."),
                                    llvm::cl::value_desc("test asm"),
                                    llvm::cl::ZeroOrMore,
                                    llvm::cl::cat(KLC3InputCat));

llvm::cl::list<string> GoldPrograms("gold",
                                    llvm::cl::desc("Gold asm file. Can have multiple."),
                                    llvm::cl::value_desc("gold asm"),
                                    llvm::cl::ZeroOrMore,
                                    llvm::cl::cat(KLC3InputCat));

llvm::cl::opt<bool> DumpIssuesToFile(
        "dump-issues-to-file",
        llvm::cl::desc("Dump issues to issues.log in brief format (default=false)"),
        llvm::cl::init(false),
        llvm::cl::cat(KLC3DebugCat));

llvm::cl::opt<unsigned int> ProgressReportInterval(
        "progress-report-interval",
        llvm::cl::desc("Report progress every ... second(s) (0 for no report, default=5)"),
        llvm::cl::init(5),
        llvm::cl::cat(KLC3DebugCat));
}

ExprBuilder *constructBuilderChain() {
    ExprBuilder *builder;
    builder = klee::createDefaultExprBuilder();
    builder = klee::createConstantFoldingExprBuilder(builder);
    return builder;
}

Solver *constructSolverChain() {
    assert(klee::CoreSolverToUse == klee::STP_SOLVER && "Only STP solver has been adapted for Int16 -> Int16 arrays");
    Solver *solver = klee::createCoreSolver(klee::CoreSolverToUse);
    solver = klee::createCexCachingSolver(solver);
    solver = klee::createCachingSolver(solver);
    solver = klee::createIndependentSolver(solver);
    return solver;
}

PathString prepareOutputPath(const PathString &basePath) {
    PathString ret;
    for (int i = 0; i <= INT_MAX; ++i) {
        PathString dir(basePath);
        llvm::sys::path::append(dir, "klc3-out-" + std::to_string(i));
        if (mkdir(dir.c_str(), 0775) == 0) {
            ret = dir;
            /*PathString klc3_last(basePath);
            llvm::sys::path::append(klc3_last, "klc3-last");
            if (((unlink(klc3_last.c_str()) < 0) && (errno != ENOENT)) ||
                symlink(dir.c_str(), klc3_last.c_str()) < 0) {
                newProgWarn() << "Failed to create klc3-last symlink" << "\n";
            }*/
            break;
        } else {
            // If error is that file exists, continue
            assert(errno == EEXIST && "Unhandled error when trying to create output path");
        }
    }
    assert(!ret.empty() && "Failed to find an available index for output path");
    return ret;
}

bool feedASMToLoader(KLC3Loader *loader, const string &filename, bool allowInputFile, bool collectCompileIssues) {
    if (!endswith(filename, ".asm")) {
        newProgWarn() << "File \"" << filename << "\" doesn't have the extension \"asm\"\n";
    }
    progInfo() << "Loading " << filename << "...\n";
    return loader->load(filename, false, allowInputFile, collectCompileIssues);
}

bool InterruptReceived = false;

void handleInterrupt() {
    InterruptReceived = true;
    // Resume normal execution
}

bool AlarmReceived = false;

void handleAlarm(int sig) {
    (void) sig;
    AlarmReceived = true;
    alarm(ALARM_INTERVAL);  // restart the timer
}

int main(int argc, char **argv) {

    /// ================================ Basic Setup ================================

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
#else
    llvm::sys::PrintStackTraceOnErrorSignal();
#endif
    klee::KCommandLine::HideAllOptions();
    klee::KCommandLine::ShowOptions(KLC3ExecutionCat);
    klee::KCommandLine::ShowOptions(KLC3ReportCat);
    klee::KCommandLine::ShowOptions(KLC3OutputCat);
    klee::KCommandLine::ShowOptions(KLC3InputCat);
    klee::KCommandLine::ShowOptions(KLC3DebugCat);

    // Set signal handlers
    llvm::sys::SetInterruptFunction(handleInterrupt);
    signal(SIGALRM, handleAlarm);

#ifdef DEBUG
    progInfo() << "KLC3 build type: Debug\n";
#else
    progInfo() << "KLC3 build type: Release\n";
#endif

    llvm::cl::ParseCommandLineOptions(argc, argv);
    MaxTime = klee::time::Span(MaxTimeRaw);
    EarlyExitTime = klee::time::Span(EarlyExitTimeRaw);

    auto executablePath = PathString(llvm::sys::fs::getMainExecutable(argv[0], (void *) main));
    llvm::sys::path::remove_filename(executablePath);

    createReportFormatter();

    /// ================================ Check for Input Files ================================

    if (InputFiles.empty() && TestPrograms.empty()) {
        newProgErr() << "no test program is given\n";
        progExit();
    }

    if (!GoldPrograms.empty() && TestPrograms.empty()) {
        newProgErr() << "gold program is given but no test program is given\n";
        progExit();
    }

    /// ================================ Prepare Builder and Solver ================================

    // Notice that we must use dynamic allocation since we can't put all these objects on the stack
    // We will use smart pointer so that we can return anywhere

    auto builder = std::unique_ptr<ExprBuilder>(constructBuilderChain());
    auto solver = std::unique_ptr<Solver>(constructSolverChain());


    /// ================================ Load (Shared) Programs and Commands ================================

    string lastTestProgramFilename;

    auto arrayCache = std::make_unique<ArrayCache>();
    auto issuePackage = std::make_unique<IssuePackage>();
    std::unique_ptr<CrossChecker> crossChecker;
    if (!GoldPrograms.empty()) {
        crossChecker = std::make_unique<CrossChecker>(builder.get(), solver.get());
    }

    auto loader = std::make_unique<KLC3Loader>(builder.get(), arrayCache.get(), issuePackage.get(), crossChecker.get());

    // Load lc3os from klc3 executable directory
    PathString lc3OSFilename(executablePath);
    llvm::sys::path::append(lc3OSFilename, "lc3os.asm");
    loader->loadLC3OS(lc3OSFilename.c_str());

    // Load (shared) input programs
    for (auto &programFilename : InputFiles) {
        feedASMToLoader(loader.get(), programFilename, true, false);
        lastTestProgramFilename = programFilename;
    }

    /// ================================ Load Gold Programs (if Any) ================================

    std::unique_ptr<KLC3Loader> goldLoader;
    if (!GoldPrograms.empty()) {
        goldLoader = std::make_unique<KLC3Loader>(*loader); // copy loader
        for (auto &programFilename : GoldPrograms) {
            feedASMToLoader(goldLoader.get(), programFilename, false, false);
        }
    }

    /// ================================ Continue Loading Test Programs (if Any) ================================

    bool testProgramsNoError = true;
    if (!TestPrograms.empty()) {
        for (auto &programFilename : TestPrograms) {
            testProgramsNoError &= feedASMToLoader(loader.get(), programFilename, false, true);
            lastTestProgramFilename = programFilename;
        }
    }

    /// ================================ Prepare Output Directory ================================

    PathString outputPath(OutputDirectory);
    if (outputPath == "none") {
        outputPath.clear();
        newProgWarn() << "Using dry-run mode\n";
    } else if (outputPath.empty()) {
        PathString basePath;
        if (!lastTestProgramFilename.empty()) {
            basePath = lastTestProgramFilename;
        } else {
            newProgErr() << "No test program or input file is given\n";
            progExit();
        }
        llvm::sys::path::remove_filename(basePath);
        outputPath = prepareOutputPath(basePath);
        progInfo() << "Output to " << outputPath << "\n";
    } else {
        if (llvm::sys::fs::exists(outputPath)) {
            if (llvm::sys::fs::is_directory(outputPath)) {
                newProgWarn() << "output directory already exists, and KLC3 won't clean the directory\n";
            } else {
                newProgErr() << outputPath << " is an existing file.\n";
                progExit();
            }
        } else {
            llvm::sys::fs::create_directory(outputPath);
        }
        progInfo() << "Output to " << outputPath << "\n";
    }



    // After this point, empty outputPath means dry-run mode

    /// ================================ Early Exit for Compile Errors ================================

    if (!testProgramsNoError) {
        // Generate report and exit
        auto generator = std::make_unique<ResultGenerator>(outputPath, loader->getLoadedPrograms(),
                                                           loader->getInputFiles(), loader->getInitPC());
        generator->generateGlobalReport(*issuePackage.get());

        if (DumpIssuesToFile && !outputPath.empty()) {
            PathString filename(outputPath);
            llvm::sys::path::append(filename, "issues.log");
            std::error_code errorCode;
            llvm::raw_fd_ostream fs(filename, errorCode, llvm::sys::fs::F_None);
            if (errorCode) {
                newProgErr() << "Failed to open " << filename << "\n";
                return 1;
            }
            ReportFormatterBrief().generateReport(fs, *issuePackage.get());
            fs.close();
        }

        llvm::llvm_shutdown();
        return 0;
    }

    /// ================================ Sanity Check on Symbols ================================

    if (!arrayCache->hasSymbolicArray()) newProgWarn() << "No symbolic variable loaded\n";

    /// ================================ Prepare Gold Program and Modules ================================

    std::unique_ptr<Executor> goldExecutor;
    std::unique_ptr<IssuePackage> goldIssuePackage;
    State *goldStartupState = nullptr;
#if ENABLE_GOLD_LOOP_COVERAGE
    StateVector goldLoopCoverageStates;
#endif

    if (!GoldPrograms.empty()) {
        goldExecutor = std::make_unique<Executor>(builder.get(), solver.get(), goldLoader->getMem());
        goldIssuePackage = std::make_unique<IssuePackage>(*issuePackage);  // make a copy for all cross check item
        assert(goldIssuePackage->getIssues().empty());

        /// ================================ Run Gold State to the First Fork Point ================================
        goldStartupState = goldExecutor->createInitState(goldLoader->getConstraints(),
                                                         goldLoader->getInitPC(),
                                                         goldIssuePackage.get());
        // Notice that only executor involves in the execution of the gold program.
        StateVector goldStartupResult;
        while (goldExecutor->step(goldStartupState, goldStartupResult, true) && goldStartupState->status == State::NORMAL) {
            assert(goldStartupResult.size() == 1 && goldStartupResult[0] == goldStartupState && "Unexpected fork");
            goldStartupResult.clear();
        }
        // Exit the loop when the state is going to fork
        if (!goldIssuePackage->getIssues().empty() || goldStartupState->status == klc3::State::BROKEN) {
            newProgErr() << "Gold program has issue! Run the gold program in the standalone mode to debug.\n";
            progExit();
        }
        timedInfo() << "Gold state warm up went through " << goldStartupState->stepCount << " steps\n";

#if ENABLE_GOLD_LOOP_COVERAGE
        progInfo() << "================================ Gold Loop Coverage ================================\n";
        {
            /// ================================ Prepare for Loop Reduction on Gold ================================

            auto goldFlowGraph = std::make_unique<FlowGraph>(goldLoader->getMem(), goldLoader->getInitPC());
            auto goldCoverageTracker = std::make_unique<CoverageTracker>(goldFlowGraph.get());
            auto goldSubroutineTracker = std::make_unique<SubroutineTracker>(goldFlowGraph.get(),
                                                                             goldIssuePackage.get());
            auto goldLoopAnalyzer = std::make_unique<LoopAnalyzer>(goldFlowGraph.get());
            for (const auto &subroutine : goldSubroutineTracker->getSubroutines()) {
                goldLoopAnalyzer->analyzeLoops(subroutine.entry,
                                               goldSubroutineTracker->getSubroutineSubgraph(subroutine.name));
            }
            assert(goldIssuePackage->getIssues().empty() && "Gold code has issues!");
            auto goldFILOSearcher = std::make_unique<PrioritizedFILOSearcher>();
            auto goldPruningSearcher = std::make_unique<PruningSearcher>(goldFlowGraph.get(),
                                                                         goldFILOSearcher.get(),
                                                                         goldLoopAnalyzer->getTopLevelLoops(),
                                                                         goldLoopAnalyzer->getAllLoops());

            /// ============================= Loop Reduction on Gold and Collect States =============================
            timedInfo() << "Loop reduction on gold program...\n";
            {
                auto globalStartTime = steady_clock::now();
                steady_clock::time_point lastReportTime = globalStartTime;

                State *goldFetchedState = goldExecutor->createInitState(goldLoader->getConstraints(),
                                                                        goldLoader->getInitPC(),
                                                                        goldIssuePackage.get());
                goldSubroutineTracker->setupInitState(goldFetchedState);
                goldPruningSearcher->push({goldFetchedState});

                goldFetchedState = goldPruningSearcher->fetch();
                while (goldFetchedState != nullptr) {
                    StateVector goldStepResult;
                    goldExecutor->step(goldFetchedState, goldStepResult);
                    for (auto &goldResultState : goldStepResult)
                        goldCoverageTracker->updateGraphAndCoverage(goldResultState);
                    for (auto &goldResultState : goldStepResult) goldSubroutineTracker->updateColors(goldResultState);
                    goldPruningSearcher->push(goldStepResult);
                    for (auto &goldResultState : goldStepResult) {
                        if (goldResultState->status == State::HALTED) {
                            /// NOTICE: make sure no in-use states are released
                            if (goldResultState->coveredNewSegment) {
                                goldLoopCoverageStates.emplace_back(goldResultState);
                            }
#if ENABLE_STATE_EARLY_RELEASE
                            else {
                                goldPruningSearcher->eraseCompletedStates(goldResultState);
                                goldExecutor->releaseState(goldResultState);
                            }
#endif
                        } else if (goldResultState->status == State::BROKEN) assert(!"Gold state break!");
                    }
#if PROGRESS_REPORT_INTERVAL_MS != -1
                    if (duration_cast<milliseconds>(steady_clock::now() - lastReportTime).count() > PROGRESS_REPORT_INTERVAL_MS) {
                        timedInfo() << "Exploring gold... "
                                    << "Total states: " << goldExecutor->getAllocatedStateCount() << " "
                                    << "(" << goldExecutor->getAliveStateCount() << " alive)" << "\n";
                        lastReportTime = steady_clock::now();
                    }
#endif
                    if (InterruptReceived) {
                        progErrs() << "INTERRUPT RECEIVED!\n";
                        goto FINISH_GOLD_LOOP_COVERAGE;
                    }
                    goldFetchedState = goldPruningSearcher->fetch();
                }
            }
            FINISH_GOLD_LOOP_COVERAGE:
            timedInfo() << "Collected " << goldLoopCoverageStates.size() << " gold states for loop coverage\n";
        }
        progInfo() << "============================ End of Gold Loop Coverage =============================\n";
#endif
    }

    /// ================================ Prepare FlowGraph ================================

    auto flowGraph = std::make_unique<FlowGraph>(loader->getMem(), loader->getInitPC());

    /// ================================ Analyze Subroutines ================================

    auto subroutineTracker = std::make_unique<SubroutineTracker>(flowGraph.get(), issuePackage.get());

    /// ================================ Analyze Loops (if Using PruningSearcher) ================================

    std::unique_ptr<LoopAnalyzer> loopAnalyzer;
    if (SearcherType == SearcherOption::Pruning) {

        // Check for improper subroutine structures
        for (const auto &node : flowGraph->allNodes()) {
            if (node->subroutineColors.size() > 1) {
                newProgWarn()
                        << "Detect improper loop structure. Can't run PruningSearcher. Backoff to PrioritizedFILO\n";
                SearcherType = SearcherOption::PrioritizedFILO;
                goto PREPARE_SEARCHER;
            }
        }

        loopAnalyzer = std::make_unique<LoopAnalyzer>(flowGraph.get(), MAXIMAL_SEGMENT_COUNT);
        bool loopAnalysisFailed = false;
        for (const auto &subroutine : subroutineTracker->getSubroutines()) {
            if (!loopAnalyzer->analyzeLoops(
                    subroutine.entry,
                    subroutineTracker->getSubroutineSubgraph(subroutine.name)
            )) {
                loopAnalysisFailed = true;
                break;
            }
        }

        if (loopAnalysisFailed) {
            newProgWarn() << "Detect at least " << loopAnalyzer->getTotalSegmentCount()
                          << " segments. Backoff to PrioritizedFILO\n";
            SearcherType = SearcherOption::PrioritizedFILO;
            goto PREPARE_SEARCHER;
        }

#if DUMP_LOOPS_TO_TERMINAL
        progInfo() << "================================ LOOPS ================================\n";
        loopAnalyzer->dump(progInfo());
        progInfo() << "============================= END OF LOOPS ============================\n";
#endif

#if DUMP_LOOPS_TO_FILE
        if (!outputPath.empty()) {
            PathString filename(outputPath);
            llvm::sys::path::append(filename, "loops.log");
            std::error_code errorCode;
            llvm::raw_fd_ostream fs(filename, errorCode, llvm::sys::fs::F_None);
            if (errorCode) {
                newProgErr() << "Failed to open " << filename << "\n";
                return 1;
            }
            loopAnalyzer->dump(fs);
            fs.close();
        }
#endif

#if VISUALIZE_LOOPS
        if (!outputPath.empty()) {
            PathString subdir(outputPath);
            llvm::sys::path::append(subdir, "loops");
            llvm::sys::fs::create_directory(subdir);
            FlowGraphVisualizer::visualizeAllLoops(subdir, flowGraph.get(), loopAnalyzer.get());
        }
#endif
    }

    PREPARE_SEARCHER:

    /// ================================ Prepare Searcher ================================

    std::unique_ptr<Searcher> searcher;
    std::unique_ptr<Searcher> innerSearcher;
    int maxSearcherLevel = 0;
    if (SearcherType == SearcherOption::SimpleFILO) {
        progInfo() << "Using searcher: SimpleFILO\n";
        maxSearcherLevel = 0;
        searcher = std::make_unique<SimpleFILOSearcher>();
    } else if (SearcherType == SearcherOption::PrioritizedFILO) {
        progInfo() << "Using searcher: PrioritizedFILO\n";
        maxSearcherLevel = 0;
        searcher = std::make_unique<PrioritizedFILOSearcher>();
    } else if (SearcherType == SearcherOption::Pruning) {
        progInfo() << "Using searcher: Pruning\n";
        if (ReactivationOperation == ReactivationOption::Stop) {
            maxSearcherLevel = 0;  // run until there is no normal states
            progInfo() << "PruningSearcher will stop before reactivation.\n";
        } else {
            maxSearcherLevel = 1;
        }
        innerSearcher = std::make_unique<PrioritizedFILOSearcher>();
        searcher = std::make_unique<PruningSearcher>(flowGraph.get(),
                                                     innerSearcher.get(),
                                                     loopAnalyzer->getTopLevelLoops(),
                                                     loopAnalyzer->getAllLoops());
    } else
        assert(!"Invalid searcher");

    /// ================================ Setup Execution Modules ================================

    auto executor = std::make_unique<Executor>(builder.get(), solver.get(), loader->getMem());

    auto coverageTracker = std::make_unique<CoverageTracker>(flowGraph.get());

    auto variableInductor = std::make_unique<VariableInductor>(builder.get(), solver.get(),
                                                               arrayCache->getAllSymbolicArrays(),
                                                               loader->getPreferences());

    unordered_map<const State *, ConstraintSet> finalConstraintSets;  // store final constraints for assignments

    /// ================================ Prepare Test Initial State ================================

    State *initState = executor->createInitState(loader->getConstraints(),
                                                 loader->getInitPC(),
                                                 issuePackage.get());
    subroutineTracker->setupInitState(initState);

    searcher->push({initState});

#if ENABLE_GOLD_LOOP_COVERAGE
    if (!GoldProgams.empty()) {
        for (const auto &goldPreState : goldLoopCoverageStates) {
            State *testPreState = executor->forkState(initState);
            testPreState->constraints = goldPreState->constraints;
            testPreState->coveredNewSegment = true;  // do not let it get postponed
            searcher->push({testPreState});
        }
    }
#endif

    /// ================================ Go ================================
    timedInfo() << "START!\n";
    alarm(ALARM_INTERVAL);  // start the timer
    {
        auto globalStartTime = klee::time::getWallTime();
        auto lastReportTime = globalStartTime;

        int divergeStateCount = 0;
        int maxStepCount = 0;
        int maxSolverCount = 0;
        long long totalInstCount = 0;

        int currentSearcherLevel = 0;
        while (currentSearcherLevel <= maxSearcherLevel) {

            // Set searcher level
            searcher->setLevel(currentSearcherLevel);

            // Run until the level is done
            State *testFetchedState = searcher->fetch();
            while (testFetchedState != nullptr) {
                assert(testFetchedState->status == klc3::State::NORMAL);

                StateVector testStepResult;  // store states from executor->step
                executor->step(testFetchedState, testStepResult);
                totalInstCount++;

                // Update statistics
                if (testFetchedState->stepCount > maxStepCount) maxStepCount = testFetchedState->stepCount;
                if (testFetchedState->solverCount > maxSolverCount) maxSolverCount = testFetchedState->solverCount;

                // FlowGraph update included, which must be before subroutineTracker update
                for (auto &testResultState : testStepResult) coverageTracker->updateGraphAndCoverage(testResultState);

                // SubroutineTracker updates must be before searcher update (may change the State)
                for (auto &testResultState : testStepResult) subroutineTracker->updateColors(testResultState);

                for (auto &testResultState : testStepResult) {

                    // Check for execution related limits
                    if (testResultState->status == State::NORMAL) {
                        ExecutionLimitChecker::checkLimitOnState(testResultState);
                        // If a state reaches its limit, it is set to BROKEN
                    }

                    if (testResultState->status == klc3::State::HALTED) {

                        subroutineTracker->postCheckState(testResultState);

                        if (!GoldPrograms.empty()) {
                            if (goldStartupState->status == klc3::State::HALTED) {
                                // goldStartupState finished running without forking

                                ConstraintSet finalConstraints = testResultState->constraints;  // copy
                                auto res = crossChecker->compare(goldStartupState, testResultState, finalConstraints);
                                if (!res.empty()) {
                                    finalConstraintSets[testResultState] = finalConstraints;
                                }

                            } else {
                                // A test state is HALTED, launch a corresponding gold state

                                State *goldInitState = goldExecutor->forkState(goldStartupState);
                                goldInitState->constraints = testResultState->constraints;  // replace constraints

                                auto goldSearcher = std::make_unique<PrioritizedFILOSearcher>();
                                goldSearcher->push({goldInitState});

                                // Run the goldSearcher till the end
                                /*
                                 * NOTICE: currently only Executor involves in the execution of the gold program. If an new
                                 * module involves, make sure it's also added to the warmup phase.
                                 */
                                unsigned goldStateCount = 1;
                                {
#if ENABLE_PARTIAL_GOLD_COMPARE
                                    set<Issue::Type> triggeredCrossIssues;
#endif
                                    State *goldFetchedState = goldSearcher->fetch();
                                    while (goldFetchedState != nullptr) {
                                        StateVector goldStepResult;  // store states from executor->step
                                        goldExecutor->step(goldFetchedState, goldStepResult);
                                        if (goldStepResult.size() > 1) {
                                            goldStateCount += goldStepResult.size() - 1;
#if INTERACTIVE_ECHO
                                            if (duration_cast<milliseconds>(
                                                    steady_clock::now() - lastReportTime).count() >
                                                PROGRESS_REPORT_INTERVAL_MS) {
                                                // This line will be refreshed continuously
                                                progInfo() << "\r";
                                                timedInfo() << "A test state diverges to " << goldStateCount
                                                            << " gold states...";
                                                lastReportTime = steady_clock::now();
                                            }
#endif
                                        }

                                        // Check for completed gold states
                                        for (auto &goldResultState : goldStepResult) {
                                            switch (goldResultState->status) {
                                                case klc3::State::HALTED: {
                                                    ConstraintSet finalConstraints = goldResultState->constraints;  // copy
                                                    auto res = crossChecker->compare(goldResultState, testResultState,
                                                                                     finalConstraints);
                                                    if (!res.empty()) {
                                                        finalConstraintSets[testResultState] = finalConstraints;
                                                    }
#if ENABLE_PARTIAL_GOLD_COMPARE
                                                    triggeredCrossIssues.insert(res.begin(), res.end());
#endif
                                                }
                                                    break;
                                                case klc3::State::BROKEN:
                                                    newProgErr()
                                                            << "Gold program has issue! Run the gold program in the standalone mode to debug.\n";
                                                    progExit();
                                                default:
                                                    break;
                                            }
                                        }

                                        // Update searcher
                                        goldSearcher->push(goldStepResult);
#if ENABLE_STATE_EARLY_RELEASE
                                        for (State *goldResultState : goldStepResult) {
                                            if (goldResultState->status == klc3::State::HALTED) {
                                                /// NOTICE: make sure no in-use states are released
                                                if (!goldResultState->triggerNewIssue) {  // can be set by crossChecker
                                                    goldSearcher->eraseCompletedStates(goldResultState);
                                                    goldExecutor->releaseState(goldResultState);
                                                }
                                            }
                                        }
#endif

#if ENABLE_PARTIAL_GOLD_COMPARE
                                        if (!triggeredCrossIssues.empty()) {
                                        // Early exit
                                        break;
                                    }
#endif
                                        if (InterruptReceived) {
                                            timedInfo() << "INTERRUPT RECEIVED!\n";
                                            goto FINISH_SEARCHER;
                                        }

                                        if (AlarmReceived) {  // now() is expensive
                                            // Do not reset AlarmReceived for outside checking code to terminate the program
                                            if (MaxTime) {
                                                if (klee::time::getWallTime() - globalStartTime > MaxTime) {
                                                    goto FINISH_SEARCHER;
                                                }
                                            }
                                        }

                                        // Fetch next gold state
                                        goldFetchedState = goldSearcher->fetch();
                                    }
                                }
                                if (!goldIssuePackage->getIssues().empty()) {
                                    newProgErr() << "Gold program has issue! "
                                                    "Run the gold program in the standalone mode to debug.\n";
                                    progExit();
                                }
                                if (goldStateCount > 1) {
#if INTERACTIVE_ECHO
                                    progInfo() << "\r";
                                    timedInfo() << "A test state diverges to... " << goldStateCount << " gold states\n";
#endif
                                    divergeStateCount++;
                                }

                            }
                        } else {
                            // No gold program supplied. Do nothing.
                        }

                    }
                }

                // Searcher takes in states of all status
                searcher->push(testStepResult);

#if ENABLE_STATE_EARLY_RELEASE
                for (auto &testResultState : testStepResult) {
                    if (testResultState->status == klc3::State::HALTED ||
                        testResultState->status == klc3::State::BROKEN) {
                        /// NOTICE: make sure no in-use states are released
                        if (testResultState->triggerNewIssue) continue;
#if VISUALIZE_SEGMENT_COVERING_PATHS || DUMP_SEGMENT_COVERING_STATES
                        if (testResultState->coveredNewSegment) continue;
#endif
                        // Release!
                        searcher->eraseCompletedStates(testResultState);
                        executor->releaseState(testResultState);
                    }
                }
#endif

                if (InterruptReceived) {
                    progErrs() << "INTERRUPT RECEIVED!\n";
                    goto FINISH_SEARCHER;
                }

                // Report progress and check for limits
                {
                    // Due to high kernel cost of now() on the server, we reduce the frequency of these time-related checks
                    if (AlarmReceived) {
                        AlarmReceived = false;

                        auto now = klee::time::getWallTime();  // this is costly on the server

                        if (ProgressReportInterval != 0) {
                            float coverage = coverageTracker->calculateCoverage();
                            if ((now - lastReportTime).toMicroseconds() / 1000000 > ProgressReportInterval) {
                                timedInfo() << "... "
                                            << "States: " << executor->getAllocatedStateCount() << " "
                                            << "(" << executor->getAliveStateCount() << " alive, "
                                            << divergeStateCount << " diverge). "
                                            << "Inst: " << totalInstCount << ". "
                                            << "Edge coverage: " << floatToString(coverage * 100, 2) << "%"
                                            << "\n";

                                lastReportTime = now;
                            }
                        }

                        // Check for global limits
                        if (MaxTime) {
                            if (now - globalStartTime > MaxTime) {
                                timedInfo() << "Exceed global test time limit " << MaxTime.toMicroseconds() / 1000000
                                            << " s. Terminating...\n";
                                break;
                            }
                        }
                        if (EarlyExitTime) {
                            if (!issuePackage->getIssues().empty() && now - globalStartTime > EarlyExitTime) {
                                timedInfo() << issuePackage->getIssues().size() << " issue(s) found. "
                                            << "Exceed global early exit time limit "
                                            << EarlyExitTime.toMicroseconds() / 1000000
                                            << " s. Terminating...\n";
                                break;
                            }
                        }
                    }
                }

                // Fetch next test state
                testFetchedState = searcher->fetch();

            }

            // Increase searcher level
            currentSearcherLevel++;
        }

        FINISH_SEARCHER:

        searcher->setLevel(0);  // so that PruningSearcher reports non-postponed state count below

        timedInfo() << "DONE! "
                    << "Total allocated states: " << executor->getAllocatedStateCount() << " "
                    << "(" << executor->getAliveStateCount() << " alive, "
                    << searcher->getNormalStates().size() << " left NORMAL, "
                    << divergeStateCount << " diverge). "
                    << "Edge coverage: " << floatToString(coverageTracker->calculateCoverage() * 100, 2) << "%"
                    << "\n";
        progInfo() << "Total time: " << (klee::time::getWallTime() - globalStartTime).toMicroseconds() / 1000000
                    << " s\n";
        progInfo() << "Total inst: " << totalInstCount << "\n";
        progInfo() << "Max step count: " << maxStepCount << "\n";
        progInfo() << "Max solver count: " << maxSolverCount << "\n";

        if (DumpIssuesToFile) {
            progInfo() << "IndependentSolver Queries: " << klee::stats::independentSolverQueries << "\n";
            progInfo() << "IndependentSolver Time: " << klee::stats::independentSolverTime << "\n";
            progInfo() << "getIndependentConstraints Time: " << klee::stats::getIndependentConstraintsTime << "\n";
            progInfo() << "IndElemSet Cache Hits: " << klee::stats::independentElementSetCacheHits << "\n";
            progInfo() << "IndElemSet Cache Misses: " << klee::stats::independentElementSetCacheMisses << "\n";
            progInfo() << "IndElemSet Cache Size: " << klee::stats::independentElementSetCacheSize << "\n";
            progInfo() << "IndElemSet Cache Lookup Time: " << klee::stats::independentElementSetCacheLookupTime << "\n";
            progInfo() << "IndElemSet Cache Construct Time: " << klee::stats::independentElementSetConstructTime << "\n";

            progInfo() << "CacheSolver Hits: " << klee::stats::queryCacheHits << "\n";
            progInfo() << "CacheSolver Misses: " << klee::stats::queryCacheMisses << "\n";

            progInfo() << "CexCacheSolver Hits: " << klee::stats::queryCexCacheHits << "\n";
            progInfo() << "CexCacheSolver Misses: " << klee::stats::queryCexCacheMisses << "\n";
            progInfo() << "CexCacheSolver Time: " << klee::stats::cexCacheTime << "\n";

            progInfo() << "STPSolver Queries: " << klee::stats::queries << "\n";
            progInfo() << "STPSolver Time: " << klee::stats::queryTime << "\n";

            progInfo() << "SymAddr Cache Hits: " << executor->symAddrCacheHits;
            if (goldExecutor) progInfo() << ", " << goldExecutor->symAddrCacheHits;
            progInfo() << "\n";
            progInfo() << "SymAddr Cache Misses: " << executor->symAddrCacheMisses;
            if (goldExecutor) progInfo() << ", " << goldExecutor->symAddrCacheMisses;
            progInfo() << "\n";
        }

    }

#if DUMP_SEGMENT_COVERING_STATES
    if (!outputPath.empty()) {
        issuePackage->setIssueTypeName(klc3::Issue::ERR_CUSTOMIZED_1, "COVERING_NEW_SEGMENT");
        for (auto &state : searcher->getCompletedStates()) {
            if (state->coveredNewSegment) {
                state->newStateIssue(klc3::Issue::ERR_CUSTOMIZED_1, nullptr);
            }
        }
    }
#endif

    /// ================================ Post Check ================================

    subroutineTracker->checkDuplicateColors();

    /// ================================ Filter Issues ================================

    // TODO: if AGGRESSIVE_FILTER_ISSUE is enabled in IssuePackage, issues are filtered in advance and here is useless

#if ENABLE_ISSUE_FILTER
    auto issueFilter = std::make_unique<IssueFilter>();
    IssuePackage filteredPackage = issueFilter->filterIssues(*issuePackage);
#else
    IssuePackage filteredPackage = *issuePackage;
#endif

    // Do not raise issue after this point

    /// ================================ Generate Result ================================

    auto generator = std::make_unique<ResultGenerator>(outputPath, loader->getLoadedPrograms(), loader->getInputFiles(),
                                                       loader->getInitPC());

    // Generate coverage graph
    if (!outputPath.empty() && GenerateFinalFlowGraph) {
        FlowGraphVisualizer::visualizeCoverage(outputPath, flowGraph.get(), coverageTracker.get());
    }

#if VISUALIZE_SEGMENT_COVERING_PATHS
    if (!outputPath.empty()) {
        StateVector coveringSegmentStates;
        PathString subdir(outputPath);
        llvm::sys::path::append(subdir, "segment_coverage");
        llvm::sys::fs::create_directory(subdir);
        for (auto &state : searcher->getCompletedStates()) {
            if (state->coveredNewSegment) {
                coveringSegmentStates.emplace_back(state);
                FlowGraphVisualizer::visualizePaths(subdir, "S" + std::to_string(state->getUID()),
                                                    flowGraph.get(), {state});
            }
        }
        progInfo() << coveringSegmentStates.size() << " covered new segments\n";
        // FlowGraphVisualizer::visualizePaths(outputPath, "segment-covering-paths", flowGraph.get(), coveringSegmentStates);
    }
#endif

    // Generate test cases
    timedInfo() << "Generating test cases and report..." << "\n";

    /// ================================ Induce Variables for States to Generate ================================

    map<const State *, Assignment> assignments;

    for (const auto &it : filteredPackage.getIssues()) {
        for (const auto &info : it.second) {
            if (info.s != nullptr) {
                // Generate each test case only for once (may be reused in multiple issues)
                if (assignments.find(info.s) == assignments.end()) {
                    if (it.first.type < klc3::Issue::RUNTIME_ISSUE_COUNT) {
                        // Runtime issues are raised during execution of test program and don't rely on comparison,
                        // So constraints of test state itself suffice.
                        assignments.emplace(info.s, variableInductor->induceVariables(info.s->constraints));
                    } else {
                        auto it2 = finalConstraintSets.find(info.s);
                        assert(it2 != finalConstraintSets.end() && "Missing final constraints");
                        assignments.emplace(info.s, variableInductor->induceVariables(it2->second));
                    }

                }
            }
        }
    }

    /// ================================ Complete all Issue Descriptions ================================

    filteredPackage.callBackForAllDesc(assignments);

    /// ================================ Generate Test Case Files ================================

    // Generate states that triggered issues
    map<const State *, string> testCaseBasenames;
    for (const auto &it : filteredPackage.getIssues()) {
        for (const auto &info : it.second) {
            if (info.s != nullptr) {
                // Generate each test case only for once (may be reused in multiple issues)
                if (testCaseBasenames.find(info.s) == testCaseBasenames.end()) {
                    testCaseBasenames[info.s] = generator->completeState(info.s, assignments[info.s], {});
                    // progInfo() << "S" << info.s->getUID() << " generated as " << testCaseBasenames[info.s] << "\n";
                }
                info.generatedCaseName = testCaseBasenames[info.s];
            }
        }
    }

    /// ================================ Generate Report ================================

    generator->generateGlobalReport(filteredPackage);

    if (DumpIssuesToFile && !outputPath.empty()) {
        PathString filename(outputPath);
        llvm::sys::path::append(filename, "issues.log");
        std::error_code errorCode;
        llvm::raw_fd_ostream fs(filename, errorCode, llvm::sys::fs::F_None);
        if (errorCode) {
            newProgErr() << "Failed to open " << filename << "\n";
            return 1;
        }
        ReportFormatterBrief().generateReport(fs, filteredPackage);
        fs.close();
    }

    /// ================================ Exit ================================

    /*
     * Should not put complex actions in destructors that requires assumptions that some dynamically allocated objects
     * are still alive, so that we don't need to care about the order of destruction.
     */

    llvm::llvm_shutdown();
    return 0;
}
