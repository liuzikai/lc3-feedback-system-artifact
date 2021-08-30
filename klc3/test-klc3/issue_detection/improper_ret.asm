; This program have an improper RET that skip one level since incorrect R7 is loaded
; KLC3 is expected to detect and give warning of WARN_IMPROPER_RET
; 2020.07.31 by liuzikai

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
    JSR SUB2
    LEA R0, SUB1_RET_OUTPUT
    PUTS
    LD R7, SUB1_R7_STORE
    RET

SUB2
    ST R7, SUB2_R7_STORE
    LEA R0, SUB2_ENTER_OUTPUT
    PUTS
    JSR SUB3
    LEA R0, SUB2_RET_OUTPUT  ; <-- this will not be reached, see below
    PUTS
    LD R7, SUB2_R7_STORE
    RET

SUB3
    ST R7, SUB3_R7_STORE
    LEA R0, SUB3_ENTER_OUTPUT
    PUTS
    ; Do nothing
    LEA R0, SUB3_RET_OUTPUT
    PUTS
    LD R7, SUB2_R7_STORE  ; <-- Here the code made some typo. Instead of loading SUB3_R7_STORE, it loads SUB2's,
                          ;     which will make RET jump back to R1 directly
    RET

; CHECK: ================ TEST CASE 0 OUT ================
; CHECK: Enter main code
; CHECK: Enter subroutine 1
; CHECK: Enter subroutine 2
; CHECK: Enter subroutine 3
; CHECK: RET to subroutine 3
; CHECK: RET to subroutine 1
; CHECK: RET to main code
; CHECK: ================ END OF TEST CASE 0 OUT ================

; CHECK: ================ REPORT ================

; CHECK: WARN_IMPROPER_RET

; CHECK-NOT: WARN_DUPLICATE_COLOR
; Should not report improper color since stack has already mess up

; CHECK-NOT: WARN_USE_UNINITIALIZED_REGISTER
; PUTS system trap stores R1, which should not trigger WARN_USE_UNINITIALIZED_REGISTER

; CHECK: ================ END OF REPORT ================

SUB1_R7_STORE .BLKW #1
SUB2_R7_STORE .BLKW #1
SUB3_R7_STORE .BLKW #1

MAIN_ENTER_OUTPUT .STRINGZ "Enter main code\n"
MAIN_RET_OUTPUT .STRINGZ "RET to main code\n"
SUB1_ENTER_OUTPUT .STRINGZ "Enter subroutine 1\n"
SUB1_RET_OUTPUT .STRINGZ "RET to subroutine 1\n"
SUB2_ENTER_OUTPUT .STRINGZ "Enter subroutine 2\n"
SUB2_RET_OUTPUT .STRINGZ "RET to subroutine 2\n"
SUB3_ENTER_OUTPUT .STRINGZ "Enter subroutine 3\n"
SUB3_RET_OUTPUT .STRINGZ "RET to subroutine 3\n"

.END

; RUN: %klc3 %s --use-forked-solver=false --lc3-out-to-terminal=true --report-to-terminal=true --output-dir=none 2>&1 | FileCheck %s