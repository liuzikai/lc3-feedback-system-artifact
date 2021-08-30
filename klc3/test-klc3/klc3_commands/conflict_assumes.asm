; KLC3: INPUT_FILE

.ORIG x3000

HALT

TEST_DATA .BLKW #1  ; KLC3: SYMBOLIC as N
                    ; the following constraints are conflict
                    ; KLC3: SYMBOLIC N > #0
                    ; KLC3: SYMBOLIC N < #0
.END

; RUN: not %klc3 %s --use-forked-solver=false --output-dir=none 2>&1 >/dev/null | FileCheck %s
; CHECK: Initial constraints are provably unsatisfiable