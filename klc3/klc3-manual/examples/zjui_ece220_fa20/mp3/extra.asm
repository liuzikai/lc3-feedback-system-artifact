.ORIG x4800

; Weekday bit vector
;   A bit vector of -1 ends the event list.
;   May assume all bit vectors have 0 bits in the high 11 bits.
;   May not assume that event bit vectors are non-zero.
; Event name pointer
;   May assume valid and unique ASCII strings
; Event slot bit vector
;   Can be anything
;   Must ignore the meaningless bit (bit 15)

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT Extra event input of the test case.
; KLC3: COMMENT Note that this file may contains additional things after the end of extra list.
; KLC3: COMMENT lease carefully identify the end of the extra list.

.BLKW #1	        ; KLC3: SYMBOLIC as EXTRA1_WEEKDAY_BV
                    ; KLC3: SYMBOLIC EXTRA1_WEEKDAY_BV >= #-1 & EXTRA1_WEEKDAY_BV <= #3 | EXTRA1_WEEKDAY_BV >= x10 & EXTRA1_WEEKDAY_BV <= x13
                    ; -1 (empty list), b00000 to b00011, b10000 to b10011

.FILL EXTRA1_NAME   ; KLC3: INPUT EXTRA1_NAME_PTR

.BLKW #1 	        ; KLC3: SYMBOLIC as EXTRA1_SLOT_BV
                    ; KLC3: SYMBOLIC EXTRA1_SLOT_BV >= #0 & EXTRA1_SLOT_BV <= #3 | EXTRA1_SLOT_BV >= xC000 & EXTRA1_SLOT_BV <= xC003 | EXTRA1_SLOT_BV >= x8000 & EXTRA1_SLOT_BV <= x8003 | EXTRA1_SLOT_BV >= x4000 & EXTRA1_SLOT_BV <= x4003
                    ; Bit 0, 1, 14 and 15 (should not matter)

                    ; KLC3: SYMBOLIC EXTRA1_WEEKDAY_BV != #0 | EXTRA1_SLOT_BV != #0
                    ; The case that won't be test


.BLKW #1	        ; KLC3: SYMBOLIC as EXTRA2_WEEKDAY_BV
                    ; KLC3: SYMBOLIC EXTRA2_WEEKDAY_BV == #-1 | EXTRA2_WEEKDAY_BV == #14
                    ; -1 (end), b01110

.FILL EXTRA2_NAME   ; KLC3: INPUT EXTRA2_NAME_PTR

.BLKW #1 	        ; KLC3: SYMBOLIC as EXTRA2_SLOT_BV
                    ; KLC3: SYMBOLIC EXTRA2_SLOT_BV > #0 & EXTRA2_SLOT_BV <= #3 | EXTRA2_SLOT_BV >= x4000 & EXTRA2_SLOT_BV <= x4003
                    ; Bit 0, 1 and 14, excluding all 0 case (already tested at EXTRA1)
                    
                    ; EXTRA2_WEEKDAY_BV won't be 0, EXTRA2_SLOT_BV won't be 0


.BLKW #1	        ; KLC3: SYMBOLIC as EXTRA3_WEEKDAY_BV
                    ; KLC3: SYMBOLIC EXTRA3_WEEKDAY_BV == #-1 | EXTRA3_WEEKDAY_BV == #24
                    ; -1 (end), b11000

.FILL EXTRA3_NAME   ; KLC3: INPUT EXTRA3_NAME_PTR

.FILL #1 	        ; KLC3: INPUT EXTRA3_SLOT_BV
                    ; Bit 0

.FILL #-1           ; Mark the end of the extra list

EXTRA1_NAME .STRINGZ "Z"            ; KLC3: INPUT EXTRA1_NAME
EXTRA2_NAME .STRINGZ "YYYYYYYYYY"   ; KLC3: INPUT EXTRA2_NAME
EXTRA3_NAME .STRINGZ ""             ; KLC3: INPUT EXTRA3_NAME

.END