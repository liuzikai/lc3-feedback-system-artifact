; ================================ KLC3 ENTRY CODE ================================

; This piece of code serves as the entry point to your PRINT_SLOT subroutine.
; The test index is stored at x2700, which is loaded into R1 before calling your subroutine.

.ORIG x3000

LDI R1, KLC3_TEST_INDEX_ADDR
JSR PRINT_SLOT

HALT  ; KLC3: INSTANT_HALT

KLC3_TEST_INDEX_ADDR  .FILL x2700

; ============================= END OF KLC3 ENTRY CODE ============================
