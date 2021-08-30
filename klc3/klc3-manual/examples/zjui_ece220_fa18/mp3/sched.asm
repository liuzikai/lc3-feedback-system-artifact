.ORIG x5000

; Event name
;   An empty string ends the event list.
;   May assume that all event labels are valid ASCII strings.
; Weekday bit vector
;   Valid combinations. Possible empty. Set the high 11 bits to 0's.
; Event slot
;   0 to 15. Report invalid slot number.
;   Report conflict.

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT Event schedule input of the test case.
; KLC3: COMMENT Note that this file may contains additional things after the end of event list.
; KLC3: COMMENT Please carefully identify the end of the event list.

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_NAME_0
                        ; KLC3: SYMBOLIC EVENT1_NAME_0 == x0 | EVENT1_NAME_0 == x41
                        ; First character: NUL (end, empty extra list) or 'A'
.FILL #0                ; KLC3: INPUT EVENT1_NAME_1
                        ; NUL

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_WEEKDAY_BV
                        ; KLC3: SYMBOLIC EVENT1_WEEKDAY_BV >= #0 & EVENT1_WEEKDAY_BV <= #3 | EVENT1_WEEKDAY_BV >= x10 & EVENT1_WEEKDAY_BV <= x13
                        ; b00000 to b00011, b10000 to b10011

.BLKW #1                ; KLC3: SYMBOLIC as EVENT1_SLOT
                        ; KLC3: SYMBOLIC EVENT1_SLOT >= #-1 & EVENT1_SLOT <= #1 | EVENT1_SLOT == #15 | EVENT1_SLOT == #16
                        ; -1 (invalid), 0, 1, 15, 16 (invalid)

.STRINGZ "BBBBBBBBBB"   ; KLC3: INPUT EVENT2_NAME

.FILL #17               ; KLC3: INPUT EVENT2_WEEKDAY_BV
                        ; b10001

.BLKW #1                ; KLC3: SYMBOLIC as EVENT2_SLOT
                        ; KLC3: SYMBOLIC EVENT2_SLOT == #0 | EVENT2_SLOT == #1 | EVENT2_SLOT == #15
                        ; 0, 1, 15

.STRINGZ ""             ; Mark the end of the schedule

.END
