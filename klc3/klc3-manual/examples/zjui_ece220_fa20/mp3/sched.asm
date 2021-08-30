.ORIG x4000

; Weekday bit vector
;   A bit vector of -1 ends the event list.
;   May assume all bit vectors have 0 bits in the high 11 bits.
;   May not assume that event bit vectors are non-zero.
; Event name
;   May not assume that event strings are non-empty.
; Event slot
;   Report invalid slot number
;   Report conflict

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT Event schedule input of the test case.
; KLC3: COMMENT Note that this file may contains additional things after the end of event list.
; KLC3: COMMENT Please carefully identify the end of the event list.

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_WEEKDAY_BV
                        ; KLC3: SYMBOLIC EVENT1_WEEKDAY_BV >= #-1 & EVENT1_WEEKDAY_BV <= #3 | EVENT1_WEEKDAY_BV >= x10 & EVENT1_WEEKDAY_BV <= x13
                        ; -1 (empty list), b00000 to b00011, b10000 to b10011

.STRINGZ "A"            ; KLC3: INPUT EVENT1_NAME

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_SLOT
                        ; KLC3: SYMBOLIC EVENT1_SLOT >= #-1 & EVENT1_SLOT <= #1 | EVENT1_SLOT == #14 | EVENT1_SLOT == #15
                        ; -1 (invalid), 0, 1, 14, 15 (invalid)

.FILL #17               ; KLC3: INPUT EVENT2_WEEKDAY_BV
                        ; b10001

.STRINGZ "BBBBBBBBBB"   ; KLC3: INPUT EVENT2_NAME

.BLKW #1                ; KLC3: SYMBOLIC as EVENT2_SLOT
                        ; KLC3: SYMBOLIC EVENT2_SLOT == #0 | EVENT2_SLOT == #1 | EVENT2_SLOT == #14
                        ; 0, 1, 14

.FILL #-1               ; Mark the end of the schedule

.END
