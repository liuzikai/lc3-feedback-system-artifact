//
// Created by liuzikai on 1/17/21.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/Common.h"
#include "klc3/Loader/Loader.h"

#include "klee/Support/OptionCategories.h"
#include "klee/Solver/Solver.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <sys/stat.h>
#include <unistd.h>
#include <string>

#include "llvm/Support/Signals.h"

using namespace klc3;

llvm::cl::opt<string> InputFilename(llvm::cl::Positional, llvm::cl::desc("<ASM filename>"));

ExprBuilder *constructBuilderChain() {
    ExprBuilder *builder;
    builder = klee::createDefaultExprBuilder();
    builder = klee::createConstantFoldingExprBuilder(builder);
    return builder;
}

FILE *objFile = nullptr;

static void writeValue(uint16_t val) {
    unsigned char out[2];
    out[0] = (val >> 8);
    out[1] = (val & 0xFF);
    fwrite(out, 2, 1, objFile);
}

int main(int argc, char **argv) {

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 9)
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
#else
    llvm::sys::PrintStackTraceOnErrorSignal();
#endif
    klee::KCommandLine::HideAllOptions();
    llvm::cl::ParseCommandLineOptions(argc, argv);

    if (!endswith(InputFilename, ".asm")) {
        progErrs() << "File \"" << InputFilename << "\" should ends with .asm\n";
        progExit();
    }

    auto builder = std::unique_ptr<ExprBuilder>(constructBuilderChain());

    auto loader = std::make_unique<KLC3Loader>(builder.get(), nullptr, nullptr, nullptr, true);

    loader->load(InputFilename, false, false, false);

    // Generate obj file just like lc3as
    objFile = fopen((InputFilename.substr(0, InputFilename.length() - 4) + ".obj").c_str(), "wb");

    writeValue(loader->getInitPC());

    uint16_t addr = loader->getInitPC();
    for (const auto &it : loader->getMem()) {
        assert(it.first == addr && "Memory value address not continuous!");
        writeValue(castConstant(it.second->e));
        addr++;
    }

    fclose(objFile);

    llvm::llvm_shutdown();
    return 0;
}
