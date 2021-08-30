.ORIG x4000

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT This file contains the test input event list.
; KLC3: COMMENT Note that this file may contains additional things after the end of event list.
; KLC3: COMMENT Please carefully identify the end of the event list.

; Weekday bit vector
;   A bit vector of -1 ends the event list.
;   May assume all bit vectors have 0 bits in the high 11 bits.
;   May not assume that event bit vectors are non-zero.
; Event name
;   May not assume that event strings are non-empty.
; Event slot
;   Report invalid slot number
;   Report conflict

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_WEEKDAY_BV
                        ; KLC3: SYMBOLIC EVENT1_WEEKDAY_BV >= #-1
                        ; KLC3: SYMBOLIC EVENT1_WEEKDAY_BV <= x1F
                        ; -1, 0 to b11111

.STRINGZ "A"            ; KLC3: INPUT EVENT1_NAME

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_SLOT
                        ; KLC3: SYMBOLIC EVENT1_SLOT >= #-1 & EVENT1_SLOT <= #1 | EVENT1_SLOT == #14 | EVENT1_SLOT == #15
                        ; -1 (invalid), 0, 1, 14, 15 (invalid)

.BLKW #1                ; KLC3: SYMBOLIC as EVENT2_WEEKDAY_BV
                        ; KLC3: SYMBOLIC EVENT2_WEEKDAY_BV == #-1 | EVENT2_WEEKDAY_BV == x1F | EVENT2_WEEKDAY_BV == #7 | EVENT2_WEEKDAY_BV == #28
                        ; -1, b11111, b00111, b11100

.STRINGZ "BBBBBBBBBB"   ; KLC3: INPUT EVENT2_NAME
.BLKW #1                ; KLC3: SYMBOLIC as EVENT2_SLOT

                        ; KLC3: SYMBOLIC EVENT2_SLOT == #0 | EVENT2_SLOT == #14

.BLKW #1                ; KLC3: SYMBOLIC as EVENT3_WEEKDAY_BV
                        ; KLC3: SYMBOLIC EVENT3_WEEKDAY_BV == #-1 | EVENT3_WEEKDAY_BV == x1F | EVENT3_WEEKDAY_BV == #3 | EVENT3_WEEKDAY_BV == #24
                        ; -1, b11111, b00011, b11000
.STRINGZ ""             ; KLC3: INPUT EVENT3_NAME
.FILL #1                ; KLC3: INPUT EVENT3_SLOT

.FILL #-1               ; Mark the end of the schedule

.END
