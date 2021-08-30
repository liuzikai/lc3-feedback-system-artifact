; This program uses JMP instead of RET
; KLC3 is expected to detect and give warning about WARN_DUPLICATE_COLOR
; 2020.04.21 by liuzikai

.ORIG x3000

MAIN
    LEA R0, MAIN_ENTER_OUTPUT
    PUTS
    JSR SUB1
    LEA R0, MAIN_RET_OUTPUT
    PUTS
    HALT

SUB1
    ST R7, SUB1_R7_STORE
    LEA R0, SUB1_ENTER_OUTPUT
    PUTS
    ; Do nothing
    LEA R0, SUB1_RET_OUTPUT
    PUTS
    LD R6, SUB1_R7_STORE  ; <-- load R7 into R6 and JMP R6 instead of RET
    JMP R6

; CHECK: ================ TEST CASE 0 OUT ================
; CHECK-NEXT: Enter main code
; CHECK-NEXT: Enter subroutine 1
; CHECK-NEXT: RET to subroutine 1
; CHECK-NEXT: RET to main code
; CHECK: --- halting the LC-3 ---
; CHECK: ================ END OF TEST CASE 0 OUT ================

; CHECK: ================ REPORT ================
; CHECK-NEXT: WARN_DUPLICATE_COLOR
; CHECK: ================ END OF REPORT ================

SUB1_R7_STORE .BLKW #1

MAIN_ENTER_OUTPUT .STRINGZ "Enter main code\n"
MAIN_RET_OUTPUT .STRINGZ "RET to main code\n"
SUB1_ENTER_OUTPUT .STRINGZ "Enter subroutine 1\n"
SUB1_RET_OUTPUT .STRINGZ "RET to subroutine 1\n"

.END

; RUN: %klc3 %s --use-forked-solver=false --lc3-out-to-terminal=true --report-to-terminal=true --output-dir=none 2>&1 | FileCheck %s