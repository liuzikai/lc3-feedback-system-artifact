KLC3 User Manual
===

KLC3 is an LC-3 symbolic execution tool built upon the [KLEE](http://klee.github.io) symbolic engine. It symbolically executes an LC-3 program, detects improper operations (accessing uninitialized memory or registers, etc.), and generates test cases, scripts and human-readable report. Given a correct (gold) version of the code, KLC3 can also perform equivalence checking between the test program and the gold for LC-3 output, memory, registers and/or executed instructions.

KLC3 extends KLEE with an LC-3 executor, an input language to specify symbolic variables and constraints in the LC-3 address space, and a few optimizations for execution performance. For technical details, please refer to (my thesis).

[A sample KLC3 report :smiley:](examples/zjui_ece220_fa20/mp2/examples/klc3-out-buggy/report.md)

# Table of Contents

- [KLC3 User Manual](#klc3-user-manual)
- [Table of Contents](#table-of-contents)
- [Introduction to Symbolic Execution](#introduction-to-symbolic-execution)
- [Issue Detection](#issue-detection)
  - [Execution Issues](#execution-issues)
    - [Possible Wild Read](#possible-wild-read)
    - [Read Uninitialized Memory](#read-uninitialized-memory)
    - [Read an Instruction as Data](#read-an-instruction-as-data)
    - [Possible Wild Write](#possible-wild-write)
    - [Overwrite Read-Only Data](#overwrite-read-only-data)
    - [Use an Uninitialized Register](#use-an-uninitialized-register)
    - [Use Uninitialized CC](#use-uninitialized-cc)
    - [Improper RET](#improper-ret)
    - [Reuse Code Across Subroutines](#reuse-code-across-subroutines)
    - [Halt in a Subroutine](#halt-in-a-subroutine)
    - [Manual HALT](#manual-halt)
    - [Execute Data as an Instruction](#execute-data-as-an-instruction)
    - [Execute Uninitialized Memory](#execute-uninitialized-memory)
    - [RET in Main Code](#ret-in-main-code)
    - [Invalid Instruction](#invalid-instruction)
    - [Overwrite an Instruction](#overwrite-an-instruction)
    - [Symbolic PC](#symbolic-pc)
    - [Reach Step Limit](#reach-step-limit)
    - [Reach Output Limit](#reach-output-limit)
  - [Behavioral Issues: Differences with the Gold Program](#behavioral-issues-differences-with-the-gold-program)
    - [Incorrect Output](#incorrect-output)
- [Output of KLC3](#output-of-klc3)
  - [Report](#report)
  - [Test Cases](#test-cases)
    - [Replay in lc3sim](#replay-in-lc3sim)
    - [Replay Pitfalls](#replay-pitfalls)
  - [Coverage Graph](#coverage-graph)
- [Using KLC3](#using-klc3)
  - [Running KLC3](#running-klc3)
  - [Restrictions on Test Code](#restrictions-on-test-code)
- [KLC3 Commands](#klc3-commands)
  - [Define Input Space](#define-input-space)
    - [Concrete Input Variables](#concrete-input-variables)
    - [Symbolic Numerical Variables](#symbolic-numerical-variables)
    - [Symbolic String-Like Variables](#symbolic-string-like-variables)
  - [Specify Memory Blocks](#specify-memory-blocks)
  - [Read-Only Data](#read-only-data)
  - [Define New Equivalence Checking Issues](#define-new-equivalence-checking-issues)
  - [Add Equivalence Checking Items](#add-equivalence-checking-items)
  - [Customize Issues](#customize-issues)
  - [Other KLC3 Commands](#other-klc3-commands)
- [Design Input Space](#design-input-space)
  - [Examples](#examples)
- [Options](#options)
  - [Limits on Test Code](#limits-on-test-code)
  - [Output Options](#output-options)
  - [Report Options](#report-options)
  - [Other Options](#other-options)
- [About](#about)

# Introduction to Symbolic Execution

Symbolic execution is a technique for analyzing a program by executing the code on *symbols* rather than on concrete values. The input space is defined with a set of symbols and a set of *constraints*. Some inputs may remain concrete. The execution engine works on *states* of the program, which represent alternative control flow paths starting from the beginning of the program. When a state encounters a branch instruction, the execution engine evaluates whether both branches are possible. If so, the state *forks* into two states, and the constraint of taking the branch is added to one state while its complement is added to the other. Other types of control flow branches, such as indirect subroutine calls and accesses to addresses that depend on symbolic values, also lead to the splitting of states. The execution engine selectively generates *test cases* with concrete values within the symbolic spaces of states that have triggered *issues*. These test cases can then be *replayed* in a debugging environment to trigger the issue in a controlled manner.

# Issue Detection
KLC3 can detect a set of improper operations of the test program and check whether its output, memory, registers and/or the last executed instruction diverge from those of the gold. Detected improper operations and divergence trigger *issues*. Issues can be categorized based on their severity, either as warnings (with names prefixed as WARN) or as errors (ERR). A state can continue execution after generating a warning, but errors are terminal. Typically, continued execution is not meaningful after an error occurs. The severity of some issues can be changed using KLC3 commands, as described in [Customize Issues](#customize-issues). 

Issues can also be categorized based on whether they are generated by the program itself or are generated by divergence from the gold program. The former, execution issues, correspond to verification of the program, and typically indicate undefined or irreproducible behavior or the possibility of a crash when the program executes. Issues arising from comparison with the gold program are behavioral issues corresponding to inequivalence between the program and the gold version, which is assumed to be correct.

## Execution Issues
Execution issues are raised during the execution of the test program itself and have nothing to do with the gold program. Most of them indicate improper operations of the test program.

### Possible Wild Read
*Identifier: WARN_POSSIBLE_WILD_READ*

The program executes a load from a memory location that contains neither an instruction nor a specific data value generated by. FILL, .STRINGZ or .BLKW. See the [Specify Memory Blocks](#specify-memory-blocks) section for more information. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Read Uninitialized Memory
*Identifier: WARN_READ_UNINITIALIZED_MEMORY*

The program executes a load from a memory location that is marked as uninitialized, for example, the uninitialized stack region. See the [Specify Memory Blocks](#specify-memory-blocks) section for more information. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Read an Instruction as Data
*Identifier: WARN_READ_INST_AS_DATA*

The program reads a memory location where an instruction resides. In KLC3, instructions and data are strictly separate and no self-modifying code is allowed (see [Restrictions on Test Code](#restrictions-on-test-code)). Loading the value of an instruction is likely to be a bug so a warning is given. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Possible Wild Write
*Identifier: WARN_POSSIBLE_WILD_WRITE*

The program writes to an unspecified memory location. See the [Specify Memory Blocks](#specify-memory-blocks) section for more information. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Overwrite Read-Only Data
*Identifier: WARN_WRITE_READ_ONLY_DATA*

The program overwrites read-only data. See the [Read-Only Data](#read-only-data) section for more information. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Use an Uninitialized Register
*Identifier: WARN_USE_UNINITIALIZED_REGISTER*

When the program starts, all registers are considered to be uninitialized (bits!). This issue arises when the test program uses a register without first writing to it. There are two exceptions: (1) `AND R0, R1, #0`, in which R1 is not considered as "used." (2) Store an uninitialized register to memory and load back to the **original** register, which can happen when a subroutine saves and loads its callee-saved registers. KLC3 tracks writes of uninitialized registers to memory and generates an issue (also of this type) if a value is later read into a different register.

The uninitialized register is set to 0 before continuing execution, which may not be the case if the corresponding test case is replayed outside of KLC3 (see [Replay Pitfalls](#replay-pitfalls)).

This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Use Uninitialized CC
*Identifier: WARN_USE_UNINITIALIZED_CC*

When the program starts, the condition codes (CC) are considered to be uninitialized. If the program executes a conditional branch (except BRnzp) before any instruction that sets CC, this issue is raised. 

CC is set to Z (zero) before continuing execution, which may not be the case if the corresponding test case is replayed outside of KLC3 (see [Replay Pitfalls](#replay-pitfalls)).

This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Improper RET
*Identifier: WARN_IMPROPER_RET*

KLC3 keeps track of layers of subroutine calls (hypothetical stack frames). This issue arises when the program returns using RET to locations other than the next instruction after the last executed JSR(R). Although there may be some reasonable usage in assembly programming to break the "stack," it's more likely to be the case that the student doesn't restore R7 properly.

This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Reuse Code Across Subroutines
*Identifier: WARN_REUSE_CODE_ACROSS_SUBROUTINES*

The program reuses a piece of code in subroutines and/or the "main" (entry) code. This sharing sometimes occurs at the end of subroutines, when students reuse code to restore registers and return, as follows:

```
SUBROUTINE1
    ; Some code
    BR RESTORE_AND_RET
SUBROUTINE2
    ; Some other code
    BR RESTORE_AND_RET

RESTORE_AND_RET
    LD R0, STORE_R0
    LD R1, STORE_R1
    ; ...
    RET

STORE_R0 .BLKW #1
STORE_R1 .BLKW #1
```

Although assembly programming doesn't forbid such behavior, the style is error-prone and fails when the subroutines call one another or are modified inconsistently. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Halt in a Subroutine
*Identifier: WARN_HALT_IN_SUBROUTINE*

The program executes HALT in a subroutine rather than in the "main" (entry) code. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Manual HALT
*Identifier: WARN_MANUAL_HALT*

The program halts outside the HALT trap by writing to the MCR register. The program writes to the MCR register manually. This issue can be disabled or set as an error using commands in the [Customize Issues](#customize-issues) section.

### Execute Data as an Instruction
*Identifier: ERR_EXECUTE_DATA_AS_INST*

The program tries to execute a data value, regardless of whether the value can be interpreted as a proper instruction or not. This issue arises when the student puts data or storage locations between instructions or forgets to RET before running into code. Its severity cannot be changed. 

KLC3 strictly differentiates data and instructions. When a state executes data as an instruction, it is terminated (this issue's severity cannot be changed). But in lc3sim, data may be interpreted as a valid instruction or a NOP, which is a [Replay Pitfall](#replay-pitfalls).


### Execute Uninitialized Memory
*Identifier: ERR_EXECUTE_UNINITIALIZED_MEMORY*

The program executes "bits." This issue's severity cannot be changed.

### RET in Main Code
*Identifier: ERR_RET_IN_MAIN_CODE*

The program returns (RET) from the "main" entry code. This issue can be disabled or set as a warning using commands in the [Customize Issues](#customize-issues) section.

### Invalid Instruction
*Identifier: ERR_INVALID_INST*

The program executes an illegal instruction. Specifically, the program executes RTI. Due to the strict separation of instructions and data in KLC3, when the program tries to execute a data value as instruction, [ERR_EXECUTE_DATA_AS_INST](#execute-data-as-an-instruction) arises instead. This issue's severity cannot be changed.

### Overwrite an Instruction
*Identifier: ERR_OVERWRITE_INST*

The program tries to overwrite an instruction (self-modifying code), which is forbidden in KLC3 (and likely to be a bug unless the assignment requires). See the [Restrictions on Test Code](#restrictions-on-test-code) section. This issue's severity cannot be changed.

### Symbolic PC
*Identifier: ERR_SYMBOLIC_PC*

The program loads symbolic data into the PC, which is forbidden in KLC3 (and likely to be a bug unless the assignment requires doing so). See the [Restrictions on Test Code](#restrictions-on-test-code) section. This issue's severity cannot be changed.


### Reach Step Limit
*Identifier: ERR_STATE_REACH_STEP_LIMIT*

One of the program's states reaches the step limit. See [Limits on Test Code](#limits-on-test-code). Given a reasonable step limit, this error suggests the student has written either an infinite loop or inefficienct code. This issue's severity cannot be changed.

### Reach Output Limit
*Identifier: ERR_STATE_REACH_OUTPUT_LIMIT*

One of the program's states reaches the output length limit. See [Limits on Test Code](#limits-on-test-code). This issue's severity cannot be changed.

## Behavioral Issues: Differences with the Gold Program

In addition to detecting runtimes issues, which arise from the test program itself, given a correct version of the code (the gold program), KLC3 can compare the display output, memory, registers and/or the last executed instruction between the test and the gold, and raise issues based on any divergent behavior.

Equivalence checking against the behavior of the gold program is performed only for states that halt normally. In other words, states that are not terminated by an execution error.

### Incorrect Output
*Identifier: ERR_INCORRECT_OUTPUT*

By default, KLC3 compares the output of the test program and the gold program and raises this issue for divergence. To disable this default output check or to define new equivalence checking issues, please refer to the [Customize Issues](#customize-issues) section.

# Output of KLC3

KLC3 automatically generates a package containing a human-readable report, test cases that trigger the detected issues, scripts to replay the test cases in lc3sim, a copy of the test program, and a coverage flow graph. Students can use the test cases and the scripts to debug their code.

A typical output package:
```
klc3-out-0
├── test0
│   ├── test0-data.asm
│   └── test0.lcs
├── test1
│   ├── test1-data.asm
│   └── test1.lcs
├── test2
│   ├── test2-data.asm
│   └── test2.lcs
├── coverage.png
├── test_program.asm
└── report.md
```
-> [A sample KLC3 output package](examples/zjui_ece220_fa20/mp2/examples/klc3-out-buggy)

## Report

The report is generated in the GitHub preferred Markdown format, which can be used as README on GitHub directly so that students can view it online.

For each detected issue, KLC3 reports a description, runtime information (associated with a test case), and a remark about possible fixes of the issue. For equivalence checking failures, correct values produced by the gold program can also be reported, depending on configuration setting (the [Customize Issues](#customize-issues) section).

-> [A sample KLC3 report](examples/zjui_ece220_fa20/mp2/examples/klc3-out-buggy/report.md)

## Test Cases

Test cases are concrete input values from the symbolic space of states that trigger issues. They are designed to be used with lc3sim so the student doesn't need to know anything about KLC3 or symbolic execution. Ideally, a test case should follow the same control path as in KLC3 and trigger the same issues when in lc3sim (except for some possible pitfalls, described below). 

### Replay in lc3sim

In the report, each issue is associated with a subdirectory containing a test case (one or more asm files) and an LC-3 script. The student may use the lc3sim script (lcs file) provided in each test case to load the test file in lc3sim. A sample lcs file:

```
f test_program
f test0/test0-data
reg PC x3000
```

Before executing the script, a student must compile the .asm files using lc3as. The script can then be executed in lc3sim using the `execute` command or `-S` command-line option (new version of lc3sim). A shell script may be provided to assist them.

### Replay Pitfalls
Ideally, a test case should trigger the same issues as its corresponding state in KLC3. However, due to implementation differences between lc3sim and KLC3, students may not get the same result when they replay the test case in lc3sim. Here are some common pitfalls.

* Uninitialized registers/CC: in KLC3, an uninitialized register is set to 0 after [WARN_USE_UNINITIALIZED_REGISTER](#use-an-uninitialized-register) is raised, whereas the register contains unspecified bits in lc3sim. Similarly, an uninitialized CC is set to Z after [WARN_USE_UNINITIALIZED_CC](#use-uninitialized-cc) is raised. As for memory, unspecified memory usually defaults to 0 in lc3sim, so there shouldn't be differences.
* Instructions vs. data in KLC3: when a state executes data or uninitialized memory in KLC3, it is immediately terminated, which may not be the case in lc3sim. For example, .STRINGZ generates ASCII characters, which are uniformly NOPs when interpreted as LC-3 instructions, and some students execute them deliberately, which should not be encouraged.
* Improper RET: lc3sim doesn't keep track of JSR/JSRR and RET, so it does nothing on improper RET.


## Coverage Graph

coverage.png illustrates the control flow graph of the test code and aggregate edge coverage of all states, which may help students debug.

Initially, we included this graph for our own debugging purposes, but students found it helpful as well, so we continue to generate it.

# Using KLC3
## Running KLC3

A correct (gold) version of code is required to perform equivalence checking. If no gold version is provided, KLC3 executes the test program alone and detects runtime issues.

To run KLC3 with a gold version, use the following command:

```
klc3 [options] <shared inputs...> --test <test asm file...> --gold <gold asm file...>
```

KLC3 directly parses asm files. All input data (concrete or symbolic) and KLC3 commands (described in the [KLC3 Commands](#klc3-commands) section) shared by the test program and the gold program are supplied after KLC3 options (described in the [Options](#options) section). 

One or more test asm files are supplied, each preceded by `--test` (for example, `--test a.asm --test b.asm`). Test asm files are loaded in order, and the PC is set to the beginning of the last asm file, which is defined to be the start of the test program. 

Similarly, supply one or more asm files of the gold program using `--gold`.

Auxiliary files, such as code provided to students, must be included in both the test list and the gold list (unless the code has already been integrated into the gold asm file). The two programs are executed using logically separate LC-3 processors and memories.

Each test case produced by KLC3 includes a copy of all input files (specified using `INPUT_FILE` described in the [Define Input Space](#define-input-space) section). The whole package contains a shared copy of the other files. But if an asm file ends with "_.asm," it won't be generated or copied, which can be used for pure KLC3 commands files or for supplying data that should not be exposed to students.

-> Samples of commands to run KLC3 can be found in [Sample Wrappers](examples).

## Restrictions on Test Code
* Self-modifying code is not allowed. When a state overwrites an instruction, [ERR_OVERWRITE_INST](#overwrite-an-instruction) is raised and the state is terminated.
* Symbolic PC values are not allowed. [ERR_SYMBOLIC_PC](#symbolic-pc) is raised when a state executes JMP/JSRR to an address that depends on the value of a symbolic variable. However, this restriction doesn't prevent the use of jump tables. A state that loads addresses from a jump table forks into multiple states in each of which the target address is a concrete value.

# KLC3 Commands

KLC3 parses asm files directly. Specifications on concrete/symbolic inputs, memory blocks, customized issues and checks are passed to KLC3 through comments called KLC3 commands. 

All KLC3 commands start with "`; KLC3: `" and are case sensitive.

## Define Input Space

An input file is an asm file that specifies concrete/symbolic variables. To mark an asm file as an input file, put the following KLC3 command at the beginning of the file.

```
; KLC3: INPUT_FILE
```

Each test case has one copy of each input file, which is automatically generated by KLC3. To include comments in the generated copies, use the following KLC3 command:

```
; KLC3: COMMENT comment to the end of line
```

Multiple `COMMENT` commands are allowed. Comments not marked in this way are not included in the generated copies.

### Concrete Input Variables

```
.FILL xABCD     ; KLC3: INPUT <variable>
.STRINGZ "ECE"  ; KLC3: INPUT <variable>
.BLKW x0042     ; KLC3: INPUT <variable>
```

`<variable>` is a symbolic name for the input variable, which should not contain spaces.

### Symbolic Numerical Variables

To specify a symbolic input variable, use the following syntax:

```
.FILL xABCD   ; KLC3: SYMBOLIC as <variable>
              ; KLC3: SYMBOLIC <variable> ==|!=|<|<=|>|>= #<decimal>|x<hex>

.BLKW <size>  ; KLC3: SYMBOLIC as <variable><size>
              ; KLC3: SYMBOLIC <variable><index> ==|!=|<|<=|>|>= #<decimal>|x<hex>
```

"`SYMBOLIC as`" defines a symbolic variable. It should directly follow .FILL or .BLKW. For .FILL, the number that follows (xABCD) is ignored. For .BLKW, the two `<size>` values must match. .STRINGZ should not be used to define symbolic variables. 

"`SYMBOLIC <expression>` add a constraint on one or more symbolic variables. Decimal or hex numbers should be in LC-3 syntax.

For example,

```
.FILL #0  ; KLC3: SYMBOLIC as SOME_NUMBER
          ; KLC3: SYMBOLIC SOME_NUMBER >= #0
          ; KLC3: SYMBOLIC SOME_NUMBER <= #14

.BLKW #2  ; KLC3: SYMBOLIC as SOME_ARRAY_OF_TWO[2]
          ; KLC3: SYMBOLIC SOME_ARRAY_OF_TWO[0] != #0
          ; KLC3: SYMBOLIC SOME_ARRAY_OF_TWO[1] == #0
```

Multiple `SYMBOLIC` commands are ANDed together to define the symbolic input space.

Each `SYMBOLIC` command supports a two-level AND-OR statement:

```
; KLC3: SYMBOLIC <exp1> & <exp2> & <exp3> | <exp4> & <exp5> | <exp6>
```

Expressions should follow the same format of `<variable>[<size>] <op> <num>`. The operator `&` means AND and the operator `|` means OR. AND has priority over OR. Only one level of expressions is allowed in a single `SYMBOLIC` command (no brackets). For example, the following two examples define the same input space:

```
; KLC3: SYMBOLIC SOME_NUMBER >= #0
; KLC3: SYMBOLIC SOME_NUMBER <= #14
```

```
; KLC3: SYMBOLIC SOME_NUMBER >= #0 & SOME_NUMBER <= #14
```

### Symbolic String-Like Variables

To define a string-like symbolic variable, use .BLKW to bound its maximum length.

```
.BLKW <size>  ; KLC3: SYMBOLIC as <variable>[<size>]
              ; KLC3: SYMBOLIC <variable> is fixed-length-string|var-length-string
``` 

`fixed-length-string` makes the variable a symbolic fixed-length string whose length is `<size>` - 1. Basically, it sets the last word to be '\0' and every other word to be in the range of printable ASCII ([0x20, 0x7E]). 

`var-length-string` works similarly except any word can be '\0'. The .BLKW block size remains fixed, regardless of actual length of the symbolic string. The program should stop reading after the end of the string.

To make KLC3 generated test cases easier to read, KLC3 prefers (but does not require) characters from A to Z if the the symbolic space allows. If only a non-alphabetic character causes an issue, the test case is still generated.

A variable-length string variable still takes the space defined by the .BLKW but words after the NUL termination are not treated as uninitialized. Programs that require consecutive strings as inputs can test with several strings of fixed but differing sizes, for example.

A string-like variable is basically an array variable, which means it can also be defined using numerical constraints described in the [Symbolic Numerical Variables](#symbolic-numerical-variables) section. For example, to define a two-digit capitalized hexadecimal string:

```
.BLKW #3  ; KLC3: SYMBOLIC as HEX_STR[3]
          ; KLC3: SYMBOLIC HEX_STR[0] >= x30 & HEX_STR[0] <= x39 | HEX_STR[0] >= x41 & HEX_STR[0] <= x46
          ; KLC3: SYMBOLIC HEX_STR[1] >= x30 & HEX_STR[1] <= x39 | HEX_STR[1] >= x41 & HEX_STR[1] <= x46
          ; KLC3: SYMBOLIC HEX_STR[2] == #0
          ; ASCII: 0 - x30, 9 - x39, A - x41, F - x46
``` 

## Specify Memory Blocks

Memory locations that are not explicitly specified with .FILL, .BLKW or .STRINGZ are considered to be unspecified in KLC3. Accesses to a memory location that contains neither an instruction nor a explicited specified data raise [WARN_POSSIBLE_WILD_READ](#possible-wild-read) and/or [WARN_POSSIBLE_WILD_WRITE](#possible-wild-write). Memory regions that student code is allowed to use must be specified explicitly.

For example, to specify a block of memory with 75 slots starting from x3800, the following file should be loaded as one of the shared inputs:

```
.ORIG x3800
.BLKW #75
.END
```

By default, any initialized memory location can be read or written. In the example above, .BLKW fills the memory block with 0's, which can be read or overwritten. But sometimes uninitialized memory blocks are needed. Writes to a memory region that is used as the stack should be allowed, but reads before writes should be recognized as accesses to uninitialized memory ([WARN_READ_UNINTIALIZED_MEMORY](#read-uninitialized-memory)).

To create an uninitialized memory block, use the following syntax.

```
.FILL/STRINGZ/BLKW ...  ; KLC3: UNINITIALIZED
```

Frequently, all memory regions in one asm file may be all uninitialized. In such case, the following command, placed at the beginning of the file, change the default flag for all regions defined in the file as uninitialized.

```
; KLC3: SET_DATA_DEFAULT_FLAG UNINITIALIZED
```

All .FILL/STRINGZ/BLKW in the file (except those marked as `READ_ONLY`) are set to be `UNINITIALIZED` by default.

## Read-Only Data

To make a concrete/symbolic value read-only, use the following syntax:

```
.FILL/STRINGZ/BLKW ...  ; KLC3: READ_ONLY
```

If a test state overwrites a `READ_ONLY` location, [WARN_WRITE_READ_ONLY_DATA](#overwrite-read-only-data) is raised, which can be used to forbid students from changing the input data.

Similarly, `READ_ONLY` can be combined with the `INPUT` or `SYMBOLIC as` commands. The order doesn't matter. The two lines in each of the following examples have the same effect respectively.

```
.FILL #0  ; KLC3: INPUT SOME_NUMBER READ_ONLY
.FILL #0  ; KLC3: READ_ONLY INPUT SOME_NUMBER

.FILL #0  ; KLC3: SYMBOLIC as SOME_NUMBER READ_ONLY
.FILL #0  ; KLC3: READ_ONLY SYMBOLIC as SOME_NUMBER
```

To make all memory regions in one asm file read-only, the following command, placed at the beginning of the file, change the default flag for all regions defined in the file as read-only.

```
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
```

All .FILL/STRINGZ/BLKW in the file (except those marked as `UNINITIALIZED`) are set to be `READ_ONLY` by default.

## Define New Equivalence Checking Issues

KLC3 allows defining new issues for equivalence checking failures:

```
; KLC3: ISSUE DEFINE ERROR|WARNING <issue identifier> "<description>" ["<hint>"]
```

`<issue identifier>` should be all uppercased and use `_` as delimiters for words.

`<description>` should start in lower case and ends without punctuation or line termination.

`<hint>` should be written in complete sentences.

The level of an equivalence checking issue has no effect on the execution, since equivalence checking is only performed when a state halts. It only affects how the issue is presented in the report.

## Add Equivalence Checking Items

To add an equivalence checking item, use one of the following commands.

```
; KLC3: CHECK OUTPUT for <issue identifier> [DUMP_GOLD|NOT_DUMP_GOLD]
; KLC3: CHECK R<lower> [to R<upper>] for <issue identifier> [DUMP_GOLD|NOT_DUMP_GOLD]
; KLC3: CHECK MEMORY <lower> [to <upper>] for <issue identifier> [DUMP_GOLD|NOT_DUMP_GOLD]
; KLC3: CHECK LAST_INST for <issue identifier> [DUMP_GOLD|NOT_DUMP_GOLD]
```

* `OUTPUT`: check the output of the test program against the gold.
* `R*`: check one or more registers.
* `MEM`: check memory from `<lower>` to `<upper>` (both inclusive). Use LC-3 syntax for addresses.
* `LAST_INST`: check the address of the last executed instruction *in the user space* (that is, excluding the OS code). This check can be used to verify whether a subroutine has returned properly (the same entry code must be used for the test program and the gold program).

`DUMP_GOLD` makes KLC3 shows correct output/registers/memory/last instruction in the report, making it easier for students to see the difference.

By default, the output is compared and targets [ERR_INCORRECT_OUTPUT](#incorrect-output) with `DUMP_GOLD`. To turn off the default output comparison, use `ISSUE SET_LEVEL ERR_INCORRECT_OUTPUT NONE`.

## Customize Issues

```
; KLC3: ISSUE SET_LEVEL <issue identifier> ERROR|WARNING|NONE
; KLC3: ISSUE SET_DESCRIPTION <issue identifier> "<description>"
; KLC3: ISSUE SET_HINT <issue identifier> "<hint>"
```

`<issue identifier>` for [built-in execution issues](#execution-issues) can be found under the titles of each issue.

For example, with `SET_LEVEL WARN_POSSIBLE_WILD_READ ERROR`, a state that reads unspecified memory is terminated at the erroneous load instruction.

All built-in execution warnings can be set to NONE, WARN or ERROR. All built-in execution errors except [ERR_RET_IN_MAIN_CODE](#ret-in-main-code) must be errors since KLC3 can't continue the execution. 

## Other KLC3 Commands

When testing a standalone subroutine, we may want to check the register values after it returns. However, HALT changes registers. To make a state instantly halt without going into the OS trap, use the following command.

```
HALT  ; KLC3: INSTANT_HALT
```

When a state executes this instruction, it halts immediately without going into the OS code, and therefore all register values are preserved. The halting message is not printed.

# Design Input Space

A well-designed input space is critical to detect issues in the test code effectively and efficiently. An overly constrained input space may not expose all bugs in the test program, while an overly general one may not be completely explored in a reasonable time (due to path explosion). The input space should be specialized to the assignment and may depend on the understanding of possible bugs that students may write. Here are a few suggestions that help in designing an input space.

* Instead of defining a symbolic variable with only one possible value, use a concrete input value, which can significantly increase KLC3 execution speed.
* Test for replication at least twice. When an operation needs to be performed repeatedly, test it at least twice. Novice programmers tend to copy and paste. It's possible that the first operation is done in one piece of code, while the subsequent operations are done in another buggy, piece of code.
* For a fixed loop, test all combinations if it doesn't lead to path explosion and the time for each test is acceptable. Otherwise, test the edge cases (the first two iterations and the last one, for example).

## Examples

* [Input space](examples/zjui_ece220_fa20/mp1) on [MP1 from ECE220 FA20 ZJUI section](http://lumetta.web.engr.illinois.edu/220-F20/mp/mp1.pdf)
* [Input space](examples/zjui_ece220_fa20/mp2) on [MP2 from ECE220 FA20 ZJUI section](http://lumetta.web.engr.illinois.edu/220-F20/mp/mp2.pdf)
* [Input space](examples/zjui_ece220_fa20/mp3) on [MP3 from ECE220 FA20 ZJUI section](http://lumetta.web.engr.illinois.edu/220-F20/mp/mp3.pdf)

# Options
## Limits on Test Code

Students can write all kinds of code. To provide in-time feedback, in addition to a properly designed input space, limits on student code are also necessary.

To limit the total running time of KLC3, use the following two options:

```
-max-time=<string>              - Stop exploring the test program and generate result after this amount of time (0 for no constraint, default)

-early-exit-time=<string>       - If this amount of time has passed and at least one issue is detected, stop exploring and generate result (0 for no constraint, default)
```

KLC3 uses the same time format as KLEE:

```
Time spans can be specified in two ways:
1. As positive real numbers representing seconds, e.g. '10', '3.5' but not 'INF', 'NaN', '1e3', '-4.5s'
2. As a sequence of natural numbers with specified units, e.g. '1h10min' (= '70min'), '5min10s' but not '3.5min', '8S'
The following units are supported: h, min, s, ms, us, ns.
```

But even when total time is limited, KLC3 may get stuck in one state, as is the case the test program contains an infinite loop, for example. The following two options can be used:

```
-max-lc3-step-count=<int>       - Terminate a state when its step count reaches ... (0 for no constraint, default)

-max-lc3-out-length=<int>       - Terminate a state when its output length reaches ... (0 for no constraint, default)  
```

Usually, the step limits on students' code should be a few times that requires by the gold program. Although reaching the step limit doesn't necessarily imply that the program is incorrect, it's reasonable to say that at least the student has implemented an inefficient approach.

## Output Options

By default, KLC3 creates a directory at the same path as the last loaded asm file named "klc3-out-\*" where "\*" is the minimal integer that doesn't conflict with existing directories.

To change the output directory, use the following option. The path is relative to the current working directory.

```
-output-dir=<string>            - Output directory that contains report and generated test cases.
                                If not specified, klc3 will output to 'klc3-out-*' directory.
                                If set as "none," no output files will be created (dry-run mode).
                                Otherwise, output will be written to the given directory.
```

To copy additional files to the output directory such as the replay script, use the following option.
```
--copy-additional-file=<file>   - Copy an additional file to the output directory, such as the replay script. Multiple such options are allow, one for a file.
  
```

There are a few other output options. Run `klc3 --help` for the full list.

## Report Options

To change the report filename, use the following option.

```
-report-basename=<string>       - Basename for report file, excluding extension (determined by report-style option) (default="report")
```

When a report is viewed online, spaces in LC-3 output may not be shown clearly. For assignments related to string processing, KLC3 replaces all spaces in LC-3 output with "_" when the following option is set.

```
-report-replace-space           - Replace each space with '_' for LC-3 output in the report (default=false)
```

Similarly, linefeeds can be replaced by a special Unicode character indicating the end of the line.

```
-report-replace-linefeed        - Replace '\n' and '\r' with unicode special character for LC-3 output in the report (default=false)
```

Run `klc3 --help` for the full list of report options.

## Other Options

There is one option inherited from KLEE that may be critical to KLC3 performance:
```
-use-forked-solver               - Run the core SMT solver in a forked process (default=true)
```
Our experiments on some virtual machines show that forking can introduce a great overhead ( kernel operations take a large portion of time). In the case, forked solver should be turned off.

# About

KLC3 user manual and asserts are distributed as associated files of KLC3 under the University of Illinois Open Source
License. See [LICENSE.TXT](../LICENSE.TXT) for details.

Assignments and LC-3 code are written by individual instructors and/or students.
