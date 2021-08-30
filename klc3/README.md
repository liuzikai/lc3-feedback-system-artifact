KLC3: KLEE on LC-3
==================

A symbolic execution engine to provide end-to-end feedback and specific test cases to students on their LC-3 programs.

Currently, the engine supports the LC-3 ISA introduced in _Introduction To Computing Systems (2nd Edition)_ by Patt And Patel.

=> [KLC3 User Manual](klc3-manual)

## Install
KLC3 makes use of KLEE's build system with a few modifications. KLC3 is configured as a CMake target built alone with
the original targets.

### Docker Image

Using KLC3 with Docker is similar to that of KLEE: [KLEE's guide](https://klee.github.io/releases/docs/v2.2/docker/).

Build the Docker image locally:

```
docker build -t klc3 .
```

To create a temporary container:
```
docker run --rm -ti --ulimit='stack=-1:-1' klc3
```

Alternatively, to create a persistent container:
```
docker run -ti --name=my_first_klc3_container --ulimit='stack=-1:-1' klc3
```

### Build from Source
Please follow the KLEE build guide: [Building KLEE with LLVM 9](https://klee.github.io/releases/docs/v2.2/build-llvm9/), with the following modification:
* Only STP solver is currently supported! KLC3 is using symbolic array with domain and range of `Int16`. We have done some modifications into KLEE's infrastructure but only for the STP solver.
* Graphviz and Flex is required to generate control flow graphs: `sudo apt install libgraphviz-dev flex libfl-dev` (Ubuntu) or `brew install graphviz flex` (macOS).

## Examples in KLC3 User Manual

KLC3 user manual contains a few examples, including LC-3 programs and wrappers.

[`get_sign`](klc3-manual/examples/get_sign) is a simple example that illustrates the issue detection capability of KLC3.
[`get_sign.asm`](klc3-manual/examples/get_sign/get_sign.asm) is an LC-3 programs that contains two trivial bugs, which
are triggered depending on the sign of an input integer.

The KLC3 output is in [`klc3-out-0`](klc3-manual/examples/get_sign/klc3-out-0). To run KLC3 yourself
(using the Docker image as example):

```
cd ~/klee_src/klc3-manual/examples/get_sign
./run_klc3.sh
```

We also provide a few real LC-3 programming assignments. We include test programs and gold programs (correct versions
of the code) written by us (we cannot provide student code).

If no gold program is supplied, KLC3 only detects problems during the execution of the test program
(execution issues in the [KLC3 manual](klc3-manual)).

To enable the equivalence checking on output, memory, registers and/or the last executed instruction
(behavioral issues in the [KLC3 manual](klc3-manual)),
KLC3 needs a gold program to run alone with a test program.

[MP1 from ECE220 FA20 ZJUI section](klc3-manual/examples/zjui_ece220_fa20/mp1) requires students to write two
subroutines `PRINT_CENTERED` and `PRINT_SLOT`.
[klc3-out-buggy_print_centered](klc3-manual/examples/zjui_ece220_fa20/mp1/print_centered/examples/klc3-out-buggy_print_centered)
shows the output of a make-up buggy example.

To run KLC3:

```
cd ~/klee_src/klc3-manual/examples/zjui_ece220_fa20/mp1/print_centered
./run_klc3.sh examples/mp1_buggy_print_centered.asm
```

The default output directory is `klc3-out-*` under the same directory as the last loaded test asm file. In this example,
the output directory is in `examples` subdirectory.

[MP2](klc3-manual/examples/zjui_ece220_fa20/mp2) is a more complicated assignment.
[Output of an buggy program](klc3-manual/examples/zjui_ece220_fa20/mp2/examples/klc3-out-buggy).

[MP3](klc3-manual/examples/zjui_ece220_fa20/mp3) is an even more complicated assignment. Executing KLC3 with the given
input space typically takes 2 to 5 minutes.

We also includes [MPs from ECE220 FA18 ZJUI section](klc3-manual/examples/zjui_ece220_fa18), which are slightly
different from those of FA20.

## Source Overview
KLC3 mostly follows the source code structure of KLEE (v2.2, described in [Developer's Guide · KLEE](https://klee.github.io/releases/docs/v2.2/docs/developers-guide/).
* `include/klc3` contains headers.
* `lib/klc3` contains most of the code.
* `tools` contains a few executable tools.
* `tests-klc3` contains small LC-3 programs that serve as automatic system tests for KLC3.
* `klc3-manual` contains the user manual and examples for instructors.

## License

KLC3 is released with the same license as KLEE (University of Illinois/NCSA
Open Source License). Please refer to [LICENSE.txt](LICENSE.TXT) for details.

Assignments and LC-3 code in the KLC3 user manual (klc3-manual) are written by
individual instructors and/or students.

The LC-3 tools package is integrated as parts of KLC3 (tools/lc3tools_release
and lib/klc3/Loader/Lexer.f), which have additional or alternate copyrights,
licenses, and/or restrictions.
