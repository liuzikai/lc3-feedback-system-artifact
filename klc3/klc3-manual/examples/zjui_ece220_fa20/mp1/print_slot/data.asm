; This file contains symbolic test index that serves as input to PRINT_SLOT, which store the index at x2700, which will
; be loaded into R1.

.ORIG x2700

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT This file contains the test input index.

.BLKW #1      ; KLC3: SYMBOLIC as R1_VALUE
              ; KLC3: SYMBOLIC R1_VALUE >= #0
              ; KLC3: SYMBOLIC R1_VALUE <= #14

.END