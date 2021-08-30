; ================================ KLC3 ENTRY CODE ================================

; This piece of code serves as the entry point to your PRINT_CENTERED subroutine.
; The test string is stored at x2700, whose address is loaded into R1 before calling your subroutine.

.ORIG x3000

LD R1, KLC3_TEST_STRING_ADDR
JSR PRINT_CENTERED

HALT  ; KLC3: INSTANT_HALT

KLC3_TEST_STRING_ADDR  .FILL x2700

; ============================= END OF KLC3 ENTRY CODE ============================
