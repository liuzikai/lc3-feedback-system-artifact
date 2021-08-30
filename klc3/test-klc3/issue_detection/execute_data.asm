; This program stores bit code of instructions as data.
; KLC3 is expected to detect and give warning about executing data as instruction
; 2020.04.21 by liuzikai

.ORIG x3000

AND R0, R0, #0

DATA_INST1  .FILL x102F  ; this is the bit code of ADD R0, R0, #15
DATA_INST2  .FILL x102F  ; this is the bit code of ADD R0, R0, #15
DATA_INST3  .FILL x102F  ; this is the bit code of ADD R0, R0, #15
DATA_INST4  .FILL x102F  ; this is the bit code of ADD R0, R0, #15
DATA_INST5  .FILL x1025  ; this is the bit code of ADD R0, R0, #5

; In latest klc3 we don't allow changes to flow graph, so the state will break after the first trial on executing data
; CHECK: ================ REPORT ================
; CHECK-NEXT: ERR_EXECUTE_DATA_AS_INST
; CHECK-SAME: AND R0, R0, #0
; CHECK-NEXT: ================ END OF REPORT ================


; If the previous two lines are executed as instructions, R0 should be 65 for now
OUT
; 'A' should shows

HALT

.END

; RUN: %klc3 %s --use-forked-solver=false --output-dir=none --report-to-terminal=true 2>&1 | FileCheck %s