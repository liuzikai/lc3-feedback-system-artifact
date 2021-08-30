; This file contains symbolic test string that serves as input to PRINT_CENTERED, which has variable length at most
; 15 characters (excluding the null termination).

.ORIG x2700

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT This file contains the test input string.

.BLKW #16  ; KLC3: SYMBOLIC as TEST_STRING[16]
           ; KLC3: SYMBOLIC TEST_STRING is var-length-string

.END