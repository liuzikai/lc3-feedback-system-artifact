.ORIG x6000

; Event name pointer
;   An empty string ends the event list.
;   May assume valid and unique ASCII strings.
; Weekday bit vector
;   Valid combinations. Possible empty. Set the high 11 bits to 0's.
; Event slot bit vector
;   Can be anything

; KLC3: INPUT_FILE
; KLC3: SET_DATA_DEFAULT_FLAG READ_ONLY
; KLC3: COMMENT Extra event input of the test case.
; KLC3: COMMENT Note that this file may contains additional things after the end of extra list.
; KLC3: COMMENT lease carefully identify the end of the extra list.

.BLKW #1            ; KLC3: SYMBOLIC as EXTRA1_NAME_PTR
                    ; KLC3: SYMBOLIC EXTRA1_NAME_PTR == x0 | EXTRA1_NAME_PTR == x600A
                    ; NULL (end, empty extra list) or pointer to "Z"

.BLKW #1	        ; KLC3: SYMBOLIC as EXTRA1_WEEKDAY_BV
                    ; KLC3: SYMBOLIC EXTRA1_WEEKDAY_BV >= #0 & EXTRA1_WEEKDAY_BV <= #3 | EXTRA1_WEEKDAY_BV >= x10 & EXTRA1_WEEKDAY_BV <= x13
                    ; b00000 to b00011, b10000 to b10011

.BLKW #1 	        ; KLC3: SYMBOLIC as EXTRA1_SLOT_BV
                    ; KLC3: SYMBOLIC EXTRA1_SLOT_BV >= #0 & EXTRA1_SLOT_BV <= #3 | EXTRA1_SLOT_BV >= x8000 & EXTRA1_SLOT_BV <= x8003
                    ; Bit 0, 1, and 15

                    ; KLC3: SYMBOLIC EXTRA1_WEEKDAY_BV != #0 | EXTRA1_SLOT_BV != #0
                    ; The case that won't be test


.BLKW #1            ; KLC3: SYMBOLIC as EXTRA2_NAME_PTR
                    ; KLC3: SYMBOLIC EXTRA2_NAME_PTR == x0 | EXTRA2_NAME_PTR == x600C
                    ; NULL (end) or pointer to "YYYYYYYYYY"

.FILL #14	        ; KLC3: INPUT EXTRA2_WEEKDAY_BV
                    ; b01110

.BLKW #1 	        ; KLC3: SYMBOLIC as EXTRA2_SLOT_BV
                    ; KLC3: SYMBOLIC EXTRA2_SLOT_BV > #0 & EXTRA2_SLOT_BV <= #3 | EXTRA2_SLOT_BV >= x8000 & EXTRA2_SLOT_BV <= x8003
                    ; Bit 0, 1 and 15, excluding all 0 case (already tested at EXTRA1)
                    
                    ; EXTRA2_WEEKDAY_BV won't be 0, EXTRA2_SLOT_BV won't be 0


.BLKW #1            ; KLC3: SYMBOLIC as EXTRA3_NAME_PTR
                    ; KLC3: SYMBOLIC EXTRA3_NAME_PTR == x0 | EXTRA3_NAME_PTR == x6017
                    ; NULL (end) or pointer to "XXX"

.FILL #24	        ; KLC3: INPUT EXTRA3_WEEKDAY_BV
                    ; b11000

.FILL #1 	        ; KLC3: INPUT EXTRA3_SLOT_BV
                    ; Bit 0


.FILL x0            ; Mark the end of the extra list


EXTRA1_NAME .STRINGZ "Z"            ; KLC3: INPUT EXTRA1_NAME
EXTRA2_NAME .STRINGZ "YYYYYYYYYY"   ; KLC3: INPUT EXTRA2_NAME
EXTRA3_NAME .STRINGZ "XXX"          ; KLC3: INPUT EXTRA3_NAME

.END