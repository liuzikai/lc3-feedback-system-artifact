; This program have an improper RET in main
; KLC3 is expected to detect and give warning of ERR_RET_IN_MAIN_CODE
; 2020.07.31 by liuzikai

.ORIG x3000

MAIN
    LEA R0, MAIN_ENTER_OUTPUT
    PUTS
    JSR SUB1
    LEA R0, MAIN_RET_OUTPUT
    PUTS
    RET  ; <-- improer use of RET, which will JMP back to PUTS
         ;     in klc3, RET in main is recognized as an error, so that the program will exit

SUB1
    ST R7, SUB1_R7_STORE
    LEA R0, SUB1_ENTER_OUTPUT
    PUTS
    ; Do nothing
    LEA R0, SUB1_RET_OUTPUT
    PUTS
    LD R7, SUB1_R7_STORE
    RET  ; this should be OK

; CHECK: ================ TEST CASE 0 OUT ================
; CHECK: Enter main code
; CHECK: Enter subroutine 1
; CHECK: RET to subroutine 1
; CHECK: RET to main code
; CHECK-NOT: --- halting the LC-3 ---
; CHECK: ================ END OF TEST CASE 0 OUT ================

; CHECK: ================ REPORT ================
; CHECK-NEXT: ERR_RET_IN_MAIN_CODE
; CHECK-SAME: 13
; CHECK-NEXT: ================ END OF REPORT ================

SUB1_R7_STORE .BLKW #1

MAIN_ENTER_OUTPUT .STRINGZ "Enter main code\n"
MAIN_RET_OUTPUT .STRINGZ "RET to main code\n"
SUB1_ENTER_OUTPUT .STRINGZ "Enter subroutine 1\n"
SUB1_RET_OUTPUT .STRINGZ "RET to subroutine 1\n"

.END

; RUN: %klc3 %s --use-forked-solver=false --lc3-out-to-terminal=true --report-to-terminal=true --output-dir=none 2>&1 | FileCheck %s