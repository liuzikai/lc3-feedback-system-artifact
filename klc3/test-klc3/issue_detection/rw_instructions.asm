; This program read/write memory address where instructions are stored
; KLC3 is expected to detect and give warning about r/w instruction
; 2020.04.21 by liuzikai

.ORIG x3000

INST1  AND R0, R0, #0
INST2  ADD R0, R0, #1

; KLC3 is expected to give warning about r/w instruction

; CHECK: ================ REPORT ================

LD R0, INST1
; CHECK: WARN_READ_INST_AS_DATA
; CHECK-SAME: LD R0, INST1
ST R0, INST2
; CHECK-NEXT: ERR_OVERWRITE_INST
; CHECK-SAME: ST R0, INST2

; CHECK: ================ END OF REPORT ================

HALT

.END

; RUN: %klc3 %s --use-forked-solver=false --output-dir=none --report-to-terminal=true 2>&1 | FileCheck %s