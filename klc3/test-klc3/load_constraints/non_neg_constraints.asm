; This is a simple program that outputs '+', '0' or '-' based on the sign of the number stored at in R1
; liuzikai 2020.04.30

; KLC3: INPUT_FILE

.ORIG x3000

LD R1, TEST_INPUT
BRn NEGATIVE_CASE
BRz ZERO_CASE
POSITIVE_CASE
    LD R0, PLUS_ASCII
    OUT
    RET  ; trigger ERR_RET_IN_MAIN_CODE so that test case is generated
ZERO_CASE
    LD R0, ZERO_ASCII
    OUT
    RET  ; trigger ERR_RET_IN_MAIN_CODE so that test case is generated
NEGATIVE_CASE
    LD R0, MINUS_ASCII
    OUT
    RET  ; trigger ERR_RET_IN_MAIN_CODE so that test case is generated


TEST_INPUT .BLKW #1   ; KLC3: SYMBOLIC as N
                      ; KLC3: SYMBOLIC N >= #0

PLUS_ASCII .FILL 43   ; '+'
ZERO_ASCII .FILL 48   ; '0'
MINUS_ASCII .FILL 45  ; '-'

; RUN: %klc3 %s --use-forked-solver=false --output-dir=none 2>&1 --lc3-out-to-terminal=true | FileCheck %s
; CHECK: TEST CASE 0 OUT
; CHECK: TEST CASE 1 OUT
; CHECK-NOT: TEST CASE 2 OUT

.END