; This program read/write data exceeded the declared boundary
; KLC3 is expected to detect and give warning about data r/w overflow
; 2020.04.21 by liuzikai

.ORIG x3000

AND R0, R0, #0
LEA R1, ARRAY

; CHECK: ================ REPORT ================

; The following two lines should execute normally
LDR R0, R1, #0
; CHECK-NOT: LDR R0, R1, #0
STR R0, R1, #0
; CHECK-NOT: STR R0, R1, #0

; The following two lines should execute normally
LDR R0, R1, #1
; CHECK-NOT: LDR R0, R1, #1
STR R0, R1, #1
; CHECK-NOT: STR R0, R1, #1

; KLC3 is expected to give warning about r/w overflow
LDR R0, R1, #2
; CHECK: WARN_POSSIBLE_WILD_READ
; CHECK-SAME: LDR R0, R1, #2
STR R0, R1, #2
; CHECK-NEXT: WARN_POSSIBLE_WILD_WRITE
; CHECK-SAME: STR R0, R1, #2

; CHECK: ================ END OF REPORT ================

HALT

ARRAY .BLKW #2

.END

; RUN: %klc3 %s --use-forked-solver=false --output-dir=none --report-to-terminal=true 2>&1 | FileCheck %s