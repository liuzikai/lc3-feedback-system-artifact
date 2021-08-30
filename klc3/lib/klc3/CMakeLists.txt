set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
find_package(Graphviz REQUIRED)
find_package(FLEX REQUIRED)

FLEX_TARGET(klc3Lexer Loader/Lexer.f ${CMAKE_CURRENT_BINARY_DIR}/lex.klc3.cpp COMPILE_FLAGS "-i")

klee_add_component(klc3Lib
        Common.cpp
        ${FLEX_klc3Lexer_OUTPUTS}
        Loader/Lexer.cpp
        Loader/Loader.cpp
        Core/State.cpp
        Core/Executor.cpp
        Core/MemoryValue.cpp
        Core/MemoryManager.cpp
        FlowAnalysis/FlowGraph.cpp
        FlowAnalysis/CoverageTracker.cpp
        FlowAnalysis/SubroutineTracker.cpp
        FlowAnalysis/FlowGraphVisualizer.cpp
        FlowAnalysis/LoopAnalyzer.cpp
        Searcher/Searcher.cpp
        Searcher/PruningSearcher.cpp
        Verification/IssuePackage.cpp
        Verification/CrossChecker.cpp
        Verification/ExecutionLimitChecker.cpp
        Generation/ReportFormatter.cpp
        Generation/IssueFilter.cpp
        Generation/VariableInductor.cpp
        Generation/ResultGenerator.cpp
        )

set(LLVM_COMPONENTS
        support
        )
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(klc3Lib PUBLIC ${LLVM_LIBS})
target_link_libraries(klc3Lib PRIVATE
        kleeBasic
        kleaverSolver
        kleeSupport
        ${GRAPHVIZ_GVC_LIBRARY}
        ${GRAPHVIZ_CGRAPH_LIBRARY}
        )
target_include_directories(klc3Lib PRIVATE ${GRAPHVIZ_INCLUDE_DIRS})
target_compile_definitions(klc3Lib PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
