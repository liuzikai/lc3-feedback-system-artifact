; By Zikai Liu

; This program does the following things:
;   1. Initialize the fixed-length field of schedule at x4000.
;   2. Translates the event list starting at x5000 into the fixed-length field
;      of schedule at x4000 and detects any invalid slot numbers and conflict
;      events.
;   3. Print the schedule using the fixed-length field, the subroutines
;      PRINT_CENTERED and PRINT_SLOT.
;
; R0: temporary
; R1: the walking pointer of the event list
; R2: the pointer to the label for the event
; R3: the bit vector of days for the event
; R4: the slot index for the event
; R5: the pointer to the schedule
; R6: the bit mask of current weekday
; R7: temporary
;

.ORIG x3000

; ONE. Initialize the schedule
JSR INIT_SCHED

; TWO. Translate the list
JSR TRANS_SCHED
; check the return value
ADD R0,R0,#0  ; set the condition codes
BRnp MAIN_DONE

; THREE. Print the schedule
JSR PRINT_SCHEDULE

MAIN_DONE
HALT

SCHED_ADDR .FILL x3800
SCHED_LENGTH .FILL #75

LIST_ADDR .FILL x4000

; INIT_SCHED
;   Initialize the fixed-length field of schedule at x3800.
;   Input: None
;   Output: None
;   Caller-save: R7
;   Callee-save: R0, R5
;   Unused: R1 - R4, R6
;
;   R0: temporary
;   R5: the pointer to the schedule
;   R7: temporary
;
INIT_SCHED

  ; preserve the registers
  ST R0,INIT_SCHED_R0
  ST R5,INIT_SCHED_R5
  ST R7,INIT_SCHED_R7

  LD R5,SCHED_ADDR
  AND R7,R7,#0  ; R7 = NULL
  LD R0,SCHED_LENGTH  ; R0 holds the remaining locations to fill
  INIT_SCHED_LOOP
    BRz INIT_SCHED_DONE
    STR R7,R5,#0  ; set the memory location to NULL
    ADD R5,R5,#1  ; increament the schedule pointer
    ADD R0,R0,#-1
    BRnzp INIT_SCHED_LOOP
  INIT_SCHED_DONE
  ; recover the registers
  LD R0,INIT_SCHED_R0
  LD R5,INIT_SCHED_R5
  LD R7,INIT_SCHED_R7
  RET

  INIT_SCHED_R0 .BLKW 1
  INIT_SCHED_R5 .BLKW 1
  INIT_SCHED_R7 .BLKW 1



; TRANS_SCHED
;   Translates the event list starting at x5000 into the fixed-length field
;   of schedule at x4000. If any invalid slot numbers or conflict is detects,
;   the subroutine will print the message and return with R0 = 1. Otherwise,
;   R0 = 0 indicades that the translation succeeded.
;
;   Input: None
;   Output: R0 (0 = success, 1 = fail)
;   Caller-save: R0, R7
;   Callee-save: R1 - R6
;   Unused: None
;
;   R0: temporary
;   R1: the walking pointer of the event list
;   R2: the pointer to the label for the event
;   R3: the bit vector of days for the event
;   R4: the slot index for the event
;   R5: the pointer to the schedule
;   R6: the bit mask of current weekday
;   R7: temporary
;
TRANS_SCHED

  ; store the registers
  ST R1,TS_R1
  ST R2,TS_R2
  ST R3,TS_R3
  ST R4,TS_R4
  ST R5,TS_R5
  ST R6,TS_R6
  ST R7,TS_R7

  LD R1,LIST_ADDR
  TS_EVENT_LOOP

    LDR R3,R1,#0  ; load the bit vector of the event
    ADD R1,R1,#1

    ; check if the list reach its end
    ADD R7,R3,#1  ; R7 <- bit vector + 1
    BRz TS_EVENT_DONE


    ADD R2,R1,#0  ; backup the starting pointer of the event name to R2

    ; calc the label length into R7
    AND R7,R7,#0  ; R7 = 0
    TS_EVENT_LEN_LOOP
      LDR R0,R1,#0  ; load the char at the walking pointer to R0
      BRz TS_EVENT_LEN_DONE
      ADD R7,R7,#1  ; increament the string length
      ADD R1,R1,#1  ; move to the next char
      BRnzp TS_EVENT_LEN_LOOP
    TS_EVENT_LEN_DONE
    ; now R1 points to the NUL, and R7 holds the string length

    
    LDR R4,R1,#1  ; load the slot index of the event
    ADD R1,R1,#2  ; point R1 to the bit vector of next event

    ; check the slot number
    ADD R4,R4,#0  ; set the condition codes
    BRn TS_EVENT_CHECK_SLOT_FAIL  ; if R4 < 0, fails
    ADD R0,R4,#-14
    BRp TS_EVENT_CHECK_SLOT_FAIL  ; if R4 > 14, fails
    BRnzp TS_EVENT_CHECK_SLOT_DONE
    TS_EVENT_CHECK_SLOT_FAIL
      ; print the event label
      ADD R0,R2,#0
      PUTS
      ; print the following error message
      LEA R0,TS_SLOT_INFO
      PUTS
      BRnzp TS_FAIL
    TS_EVENT_CHECK_SLOT_DONE

    JSR FILL_DAYS
    ; check the return value
    ADD R0,R0,#0
    BRz TS_EVENT_NEXT

    ; there is a conflict

    ; print the event label
    ADD R0,R2,#0
    PUTS
    ; print the following error message
    LEA R0,TS_CONFLICT_INFO
    PUTS
    BRnzp TS_FAIL

    TS_EVENT_NEXT
    BRnzp TS_EVENT_LOOP

  TS_EVENT_DONE
  AND R0,R0,#0  ; R0 = 0
  BRnzp TS_DONE

  TS_FAIL
  AND R0,R0,#0
  ADD R0,R0,#1  ; R0 = 1
  ; continue to TS_DONE

  TS_DONE
  ; recover the registers
  LD R1,TS_R1
  LD R2,TS_R2
  LD R3,TS_R3
  LD R4,TS_R4
  LD R5,TS_R5
  LD R6,TS_R6
  LD R7,TS_R7
  RET

  TS_SLOT_INFO .STRINGZ " has an invalid slot number.\n"
  TS_CONFLICT_INFO .STRINGZ " conflicts with an earlier event.\n"

  TS_R1 .BLKW 1
  TS_R2 .BLKW 1
  TS_R3 .BLKW 1
  TS_R4 .BLKW 1
  TS_R5 .BLKW 1
  TS_R6 .BLKW 1
  TS_R7 .BLKW 1


; FILL_DAYS
;   This subroutine first checks whether there are conflicts for the given
;   event. If there is a conflict, the subroutine returns with R0 = 1. If not,
;   R2 is stored to schedule and the subroutine returns with R0 = 0.
;
;   Input: R2: pointer to be stored in corresponding location in schedule
;          R3: the day vector of the event
;          R4: the slot index of the event
;   Output: R0 (0 = success, 1 = fail)
;   Caller-save: R0, R7
;   Callee-save: R4, R5, R6
;   Unchanged: R1, R2, R3
;
;   R0: temporary
;   R5: the pointer to the schedule
;   R6: the bit mask of current weekday
;   R7: temporary
;
FILL_DAYS

  ; store the registers
  ST R4,FD_R4
  ST R5,FD_R5
  ST R6,FD_R6
  ST R7,FD_R7

  ; make R5 point to the Fri of the slot of current event
  LD R5,SCHED_ADDR
  ADD R4,R4,#0  ; set the condition codes
  FD_INIT_R5_LOOP
    BRz FD_INIT_R5_DONE
    ADD R5,R5,#5  ; make R5 points to the Mon of next slot
    ADD R4,R4,#-1
    BRnzp FD_INIT_R5_LOOP
  FD_INIT_R5_DONE
  ADD R5,R5,#4

  ; CHECK_LOOP: check whether there is conflict
  AND R6,R6,#0
  ADD R6,R6,#1 ; R6 = 1
  AND R7,R7,#0
  ADD R7,R7,#5 ; R7 holds the remaining weekdays to process
  FD_CHECK_LOOP
    BRz FD_CHECK_DONE

    AND R0,R3,R6  ; check whether the event is on the day
    BRz FD_CHECK_NEXT  ; if not, check next weekday

    LDR R0,R5,#0  ; check whether there is a conflict
    BRz FD_CHECK_NEXT  ; no conflict, continue

    ; there is a conflict
    BRnzp FD_FAIL

    FD_CHECK_NEXT
    ADD R5,R5,#-1  ; make R5 points to the previous weekday
    ADD R6,R6,R6  ; R6 <<= 1
    ADD R7,R7,#-1
    BRnzp FD_CHECK_LOOP
  FD_CHECK_DONE

  ; FILL_LOOP: fill R2 into schedule without checks
  ADD R5,R5,#5  ; recover R5
  AND R6,R6,#0
  ADD R6,R6,#1 ; R6 = 1
  AND R7,R7,#0
  ADD R7,R7,#5 ; R7 holds the remaining weekdays to process
  FD_FILL_LOOP
    BRz FD_FILL_DONE

    AND R0,R3,R6  ; check whether the event is on the day
    BRz FD_FILL_NEXT  ; if not, check next weekday

    STR R2,R5,#0  ; put the start of the event label to the field

    FD_FILL_NEXT
    ADD R5,R5,#-1  ; make R5 points to the previous weekday
    ADD R6,R6,R6  ; R6 <<= 1
    ADD R7,R7,#-1
    BRnzp FD_FILL_LOOP
  FD_FILL_DONE

  AND R0,R0,#0
  BRnzp FD_DONE

  FD_FAIL
  AND R0,R0,#0
  ADD R0,R0,#1  ; R0 = 1

  FD_DONE
  ; recover the registers
  LD R4,FD_R4
  LD R5,FD_R5
  LD R6,FD_R6
  LD R7,FD_R7
  RET

  FD_R4 .BLKW 1
  FD_R5 .BLKW 1
  FD_R6 .BLKW 1
  FD_R7 .BLKW 1


; PRINT_SCHEDULE
;   Print the schedule using the fixed-length field, the subroutines using
;   PRINT_CENTERED and PRINT_SLOT.
;
;   Input: None
;   Output: None
;   Caller-save: R7
;   Callee-save: R0, R1, R4, R5, R6
;   Unused: R2, R3
;
;   R0: temporary
;   R1: input for PRINT_CENTERED
;   R4: the slot index for the event
;   R5: the pointer to the schedule
;   R6: temporary
;
PRINT_SCHEDULE

  ; store the registers
  ST R0,PS_R0
  ST R1,PS_R1
  ST R4,PS_R4
  ST R5,PS_R5
  ST R6,PS_R6
  ST R7,PS_R7

  ; print the header
  LEA R1,PS_EMPTY_STR ; print the first empty blank
  JSR PRINT_CENTERED
  ; print the following weekdays
  LEA R1,PS_WEEKDAY_STR
  AND R6,R6,#0
  ADD R6,R6,#5  ; R6 holds the remaining days to print
  PS_HEADER_DAY_LOOP
    BRz PS_HEADER_DAY_DONE

    ; print the separator string
    LD R0,PS_SEPARATOR_ASCII
    OUT

    ; print the weekday string
    JSR PRINT_CENTERED

    ADD R1,R1,#10  ; each string use 10 memory locations (including NUL)
    ADD R6,R6,#-1
    BRnzp PS_HEADER_DAY_LOOP
  PS_HEADER_DAY_DONE

  ; print the linefeed
  LD R0,PS_NEWLINE_ASCII
  OUT

  ; print the main body of the schedule
  LD R5,SCHED_ADDR
  AND R4,R4,#0  ; R4 now holds the slot index
  PS_EACH_SLOT_LOOP
    ADD R0,R4,#-15
    BRz PS_EACH_SLOT_DONE  ; if R0 == 15, break

    ; print the leading slot
    ADD R1,R4,#0  ; copy R4 into R1
    JSR PRINT_SLOT

    AND R6,R6,#0
    ADD R6,R6,#5 ; R6 holds the remaining weekdays to process
    PS_EACH_DAY_LOOP
      BRz PS_EACH_DAY_DONE

      ; print the separator string
      LD R0,PS_SEPARATOR_ASCII
      OUT

      ; check whether the day is empty
      LDR R0,R5,#0
      BRz PS_EACH_DAY_EMPTY
      PS_EACH_DAY_NON_EMPTY
        LDR R1,R5,#0
        JSR PRINT_CENTERED
        BRnzp PS_EACH_DAY_NEXT
      PS_EACH_DAY_EMPTY
        LEA R1,PS_EMPTY_STR
        JSR PRINT_CENTERED

      PS_EACH_DAY_NEXT
      ADD R5,R5,#1  ; increament the schedule pointer
      ADD R6,R6,#-1
      BRnzp PS_EACH_DAY_LOOP
    PS_EACH_DAY_DONE

    ; print the linefeed
    LD R0,PS_NEWLINE_ASCII
    OUT

    ADD R4,R4,#1
    BRnzp PS_EACH_SLOT_LOOP
  PS_EACH_SLOT_DONE

  ; recover the registers
  LD R0,PS_R0
  LD R1,PS_R1
  LD R4,PS_R4
  LD R5,PS_R5
  LD R6,PS_R6
  LD R7,PS_R7

  RET

  PS_EMPTY_STR .STRINGZ ""
  PS_SEPARATOR_ASCII .FILL x7C  ; ascii '|'
  PS_NEWLINE_ASCII .FILL x0A
  PS_WEEKDAY_STR  ; each string use 10 memory locations (including NUL)
    .STRINGZ "  MONDAY "
    .STRINGZ " TUESDAY "
    .STRINGZ "WEDNESDAY"
    .STRINGZ " THURSDAY"
    .STRINGZ "  FRIDAY "

  PS_R0 .BLKW 1
  PS_R1 .BLKW 1
  PS_R4 .BLKW 1
  PS_R5 .BLKW 1
  PS_R6 .BLKW 1
  PS_R7 .BLKW 1





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
