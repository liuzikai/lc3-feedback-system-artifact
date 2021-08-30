; ================================ KLC3 ENTRY CODE ================================

; This piece of code serves as the entry point to your PRINT_SLOT subroutine.
; The test index is stored at x2700, which is loaded into R1 before calling your subroutine.

.ORIG x3000

LDI R1, KLC3_TEST_INDEX_ADDR
JSR PRINT_SLOT

HALT  ; KLC3: INSTANT_HALT

KLC3_TEST_INDEX_ADDR  .FILL x2700

; ============================= END OF KLC3 ENTRY CODE ============================

; By Zikai Liu

; PRINT_SLOT
;   This subroutine prints "   XXXX  " to the screen, whose hour is the integer
;   stored in R1. It will preserves all registers. It simulates the operation of
;   dividing 10 to get the quotient (the first digit) and the remainder
;   (the second digit).
;
;   R0: holds the integer of the first digit
;   R1: holds the integer of the second digit
;   R2: holds the ASCII '0'
;
PRINT_SLOT

  ; store R0, R1, R2 and R7 into memory for later recovery
  ST R0,PRINT_SLOT_R0
  ST R1,PRINT_SLOT_R1
  ST R2,PRINT_SLOT_R2
  ST R7,PRINT_SLOT_R7

  ; print the prefix string
  LEA R0,PRINT_SLOT_PREFIX
  PUTS

  ADD R1,R1,#6  ; map R1 to 6:00 to 20:00
  AND R0,R0,#0  ; R0 = 0

  PRINT_SLOT_MINUS_TEN
    ADD R0,R0,#1  ; R0 += 1
    ADD R1,R1,#-10  ; R1 -= 10
    BRzp PRINT_SLOT_MINUS_TEN  ; keep minus ten if R1 >= 0

  PRINT_SLOT_PRINT
    ; since PRINT_SLOT_MINUS_TEN stops the first time R1 < 0, we should recover
    ;   R0 and R1 to one loop before, so that R0 holds the integer of the first
    ;   digit, and R1 holds the second content
    ADD R0,R0,#-1  ; R0 -= 1
    ADD R1,R1,#10  ; R1 += 10

    LD R2,PRINT_SLOT_ASCII_ZERO  ; put ASCII '0' into R2
    ; print the first digit
    ADD R0,R0,R2
    OUT
    ; print the second digit
    ADD R0,R1,R2
    OUT
    ; print the remainder string
    LEA R0,PRINT_SLOT_REMAINER_STR
    PUTS

  ; recover R0, R1 and R2
  LD R0,PRINT_SLOT_R0
  LD R1,PRINT_SLOT_R1
  LD R2,PRINT_SLOT_R2
  LD R7,PRINT_SLOT_R7

  RET

  PRINT_SLOT_ASCII_ZERO .FILL x30  ; ASCII '0'
  PRINT_SLOT_R0 .BLKW 1  ; temporary storage of R0
  PRINT_SLOT_R1 .BLKW 1  ; temporary storage of R1
  PRINT_SLOT_R2 .BLKW 1  ; temporary storage of R2
  PRINT_SLOT_R7 .BLKW 1  ; temporary storage of R7
  PRINT_SLOT_PREFIX .STRINGZ "   "
  PRINT_SLOT_REMAINER_STR .STRINGZ "00  "

; PRINT_CENTERED
;   This subroutine maps the string in R1 to 9 characters with possibly leading
;   and trailing spaces, and then print it to the screen.
;
;   R0: temporary, and interface with PRINT_SPACES
;   R1: holds the string pointer
;   R2: holds the string length
;   R7: temporary
;
PRINT_CENTERED

  ; store R0 and R1 into memory for later recovery
  ST R0,PRINT_CENTERED_R0
  ST R1,PRINT_CENTERED_R1
  ST R2,PRINT_CENTERED_R2
  ST R7,PRINT_CENTERED_R7

  AND R2,R2,#0  ; R2 = 0
  PRINT_CENTERED_CHECK
    LDR R0,R1,#0
    BRz PRINT_CENTERED_CHECK_DONE
    ADD R2,R2,#1  ; R2 += 1
    ADD R1,R1,#1  ; move to the next character
    BRnzp PRINT_CENTERED_CHECK
  PRINT_CENTERED_CHECK_DONE

  ADD R2,R2,#-9
  BRnz PRINT_CENTERED_LE_NINE

  ; string length is greater than 6
  PRINT_CENTERED_G_NINE
    LD R1,PRINT_CENTERED_R1
    ; R2 = 9
    AND R2,R2,#0
    ADD R2,R2,#9
    PRINT_CENTERED_FOO
      BRz PRINT_CENTERED_DONE
      LDR R0,R1,#0
      OUT
      ADD R1,R1,#1
      ADD R2,R2,#-1
      BRnzp PRINT_CENTERED_FOO

  ; string length is less or equal to 9
  PRINT_CENTERED_LE_NINE
    ADD R2,R2,#9  ; recover R2

    ; read the number of leading spaces into R0
    LEA R7, PRINT_CENTERED_LEADING_COUNT
    ADD R7,R7,R2
    LDR R0,R7,#0  ; put the number of space into R0
    ; print the leading spaces
    JSR PRINT_SPACES

    ; print the original string
    LD R0,PRINT_CENTERED_R1
    PUTS

    ; read the number of trailing space into R0
    LEA R7, PRINT_CENTERED_TRAILING_COUNT
    ADD R7,R7,R2
    LDR R0,R7,#0 ; put the number of space into R0
    ; print the trailing spaces
    JSR PRINT_SPACES

  PRINT_CENTERED_DONE
  
  ; recover R0 and R1
  LD R0,PRINT_CENTERED_R0
  LD R1,PRINT_CENTERED_R1
  LD R2,PRINT_CENTERED_R2
  LD R7,PRINT_CENTERED_R7

  RET

  PRINT_CENTERED_R0 .BLKW 1  ; temporary storage of R0
  PRINT_CENTERED_R1 .BLKW 1  ; temporary storage of R1
  PRINT_CENTERED_R2 .BLKW 1  ; temporary storage of R1
  PRINT_CENTERED_R7 .BLKW 1  ; temporary storage of R7
  PRINT_CENTERED_LEADING_COUNT
    .FILL #5
    .FILL #4
    .FILL #4
    .FILL #3
    .FILL #3
    .FILL #2
    .FILL #2
    .FILL #1
    .FILL #1
    .FILL #0
  PRINT_CENTERED_TRAILING_COUNT
    .FILL #4
    .FILL #4
    .FILL #3
    .FILL #3
    .FILL #2
    .FILL #2
    .FILL #1
    .FILL #1
    .FILL #0
    .FILL #0


; PRINT_SPACES
;   This subroutine prints R0 number of space.
;
;   R0: interface, and temporary
;   R6: holds the number of remaining spaces
;
PRINT_SPACES

  ; store R0, R6 and R7 into memory
  ST R0,PRINT_SPACES_R0
  ST R6,PRINT_SPACES_R6
  ST R7,PRINT_SPACES_R7

  ADD R6,R0,#0  ; move R0 into R6
  LD R0,PRINT_SPACES_ASCII_SPACE
  ADD R6,R6,#0  ; set condition codes of R6

  PRINT_SPACES_CHECK
    BRnz PRINT_SPACES_DONE
    OUT  ; print a space
    ADD R6,R6,#-1  ; R6 -= 1
    BRnzp PRINT_SPACES_CHECK

  PRINT_SPACES_DONE
  ; recover R0, R6 and R7
  LD R0,PRINT_SPACES_R0
  LD R6,PRINT_SPACES_R6
  LD R7,PRINT_SPACES_R7

  RET

  PRINT_SPACES_R0 .BLKW 1 ; temporary storage of R0
  PRINT_SPACES_R6 .BLKW 1 ; temporary storage of R6
  PRINT_SPACES_R7 .BLKW 1 ; temporary storage of R7
  PRINT_SPACES_ASCII_SPACE .FILL x20


.END
