# End-to-End Automatic Feedback System on LC-3 Assembly Programs
This replication package is provided as a standalone artifact and thus contains everything necessary to reproduce the results in:

Z. Liu, T. Liu, Q. Li, W. Luo, and S. Lumetta, "End-to-End Automation of Feedback on Student Assembly Programs," in *Proceedings of the 36th ACM/IEEE International Conference on Automated Software Engineering*, ser. ASE 2021.

with the exception of student code samples, which are the property of the students (the package does include several sample solutions for each sample assignment--see the [klc3/klc3-manual/examples](klc3/klc3-manual/examples) subdirectory).

Currently, the system supports the LC-3 ISA in _Introduction To Computing Systems (2nd Edition)_ by Patt And Patel.

This package includes a series of tools. Please refer to the individual subdirectories for READMEs, setup guides and licenses.

## [LC-3 VSCode Extension](lc3-vscode)
Static analysis tool for correctness and style when student edits their code.

For more recent updates and extensions to this tool, please check [Liqi1003/lc3_vscode](https://github.com/Liqi1003/lc3_vscode).

## [KLC3](klc3)
"[KLEE](klc3/README-KLEE.md) on LC-3," a symbolic LC-3 execution engine to provide feedback and test cases to students.

For more recent updates and extensions to this tool, please check [liuzikai/klc3](https://github.com/liuzikai/klc3).

## [KLC3 Queue System](klc3-queue-system)
Trigger dynamic analysis as students push their code to Git.

## [LC-3 Browser-Based Tools](lc3-webtool)
LC-3 simulation and debug tools based on WebAssembly.

## [LC-3 Executable-Based Tools](klc3/tools/lc3tools_release)
LC-3 simulation and debug tools as executables.
