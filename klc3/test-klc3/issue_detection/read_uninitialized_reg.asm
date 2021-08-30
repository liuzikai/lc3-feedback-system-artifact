; This program uses uninitialized registers
; KLC3 is expected to detect and give warning about it
; 2020.08.02 by liuzikai

.ORIG x3000

MAIN
    JSR SUB1
    HALT

SUB1
    ST R1, SUB1_R1_STORE
    ST R2, SUB1_R2_STORE
    ST R3, SUB1_R3_STORE
    ST R7, SUB1_R7_STORE

    JSR SUB2

    AND R0, R0, #0        ; this is OK. Although it involves reading R0, the result is always 0
    AND R1, R1, #1        ; this is BAD
    ADD R1, R1, #1        ; R1 should be set as 0 after giving warning last step, so this should be ok

    LD R4, SUB1_R1_STORE  ; this is BAD, loading value from uninitialized register to another register other than R1
                          ; and klc3 should be able to detect the origin ST R1, SUB1_R1_STORE

    LEA R0, SUB1_R2_STORE
    LDR R4, R0, #0        ; similar, but using LDR

    LEA R0, SUB1_R3_STORE
    ST R0, TEMP_LOC
    LDI R4, TEMP_LOC      ; similar, but using LDI

    LD R4, SUB1_R1_STORE  ; since we only want to warn once, this should not produce warning again

    NOT R5, R6            ; this is BAD

    LD R1, SUB1_R1_STORE
    LD R2, SUB1_R2_STORE
    LD R3, SUB1_R3_STORE
    LD R7, SUB1_R7_STORE
    RET

SUB2
    ST R1, SUB2_R1_STORE  
    ST R2, SUB2_R2_STORE
    ST R3, SUB2_R3_STORE
    ST R7, SUB2_R7_STORE  
    ; Do nothing
    LD R1, SUB2_R1_STORE
    LD R2, SUB2_R2_STORE
    LD R3, SUB2_R3_STORE
    LD R7, SUB2_R7_STORE
    RET


SUB1_R1_STORE .BLKW #1
SUB1_R2_STORE .BLKW #1
SUB1_R3_STORE .BLKW #1
SUB1_R7_STORE .BLKW #1
SUB2_R1_STORE .BLKW #1
SUB2_R2_STORE .BLKW #1
SUB2_R3_STORE .BLKW #1
SUB2_R7_STORE .BLKW #1

TEMP_LOC .BLKW #1

.END

; CHECK: ================ REPORT ================

; Enter SUB1
; Even R1/R2 is uninitialized, klc3 should reserve its warning since this is storing registers
; CHECK-NOT: ST R1, SUB1_R1_STORE
; CHECK-NOT: ST R2, SUB1_R2_STORE
; CHECK-NOT: ST R7, SUB1_R7_STORE

; Enter SUB2
; CHECK-NOT: ST R1, SUB2_R1_STORE
; CHECK-NOT: ST R2, SUB2_R2_STORE
; CHECK-NOT: ST R7, SUB2_R7_STORE
; CHECK-NOT: LD R1, SUB2_R1_STORE
; CHECK-NOT: LD R2, SUB2_R2_STORE
; CHECK-NOT: LD R7, SUB2_R7_STORE

; Back to SUB1
; CHECK-NOT: AND R0, R0, #0

; CHECK: WARN_USE_UNINITIALIZED_REGISTER
; CHECK: AND R1, R1, #1

; CHECK-NOT: ADD R1, R1, #1

; CHECK: WARN_USE_UNINITIALIZED_REGISTER
; CHECK: LD R4, SUB1_R1_STORE

; CHECK: WARN_USE_UNINITIALIZED_REGISTER
; CHECK: LDR R4, R0, #0

; CHECK: WARN_USE_UNINITIALIZED_REGISTER
; CHECK: LDI R4, TEMP_LOC

; CHECK-NOT: LD R4, SUB1_R1_STORE

; CHECK: WARN_USE_UNINITIALIZED_REGISTER
; CHECK: NOT R5, R6

; CHECK: ================ END OF REPORT ================

; RUN: %klc3 %s --use-forked-solver=false --output-dir=none --report-to-terminal=true 2>&1 | FileCheck %s