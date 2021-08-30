; By Steve Lumetta

	.ORIG	x3000


; changed to subroutines to make it easier to add/remove parts
	JSR	CLEAR_SCHEDULE
	JSR	TRANSLATE
	JSR	ADD_EXTRA
	ADD	R0,R0,#0
	BRz	NOPRINT
	JSR	PRINT_SCHEDULE
	BRnzp	CODE_DONE
NOPRINT LEA	R0,FAILURE
	PUTS
CODE_DONE
	HALT
FAILURE	.STRINGZ "Could not fit all events into schedule.\n"

; subroutine to try to add extra events to a schedule
; returns R0=1 on success, R0=0 on failure

ADD_EXTRA

; registers used in ADD_EXTRA
;   R0 = extra event pointer
;   R1 = temporary (used for copying and bit masks)
;   R2 = row pointer in schedule
;   R4 = temporary
;   R6 = stack pointer

; events on the stack appear as follows...
;   +0 = string pointer for event
;   +1 = day bitmap
;   +2 = hour bitmap WITHOUT currently mapped hour
;   +3 = current hour value (at which event has been inserted)
;   +4 = bit vector value for current hour (at +3)
;

	ST	R7,AE_R7	; save R7

	LD	R0,EXTRA
	LD	R6,ESTACK

INSERT_EVENT
; start by trying to insert the next event
	LDR	R1,R0,#0
	BRnp	HAVE_MORE
	; no more events--success!
	ADD	R0,R1,#1
	BRnzp	AE_DONE

HAVE_MORE
	; copy event to stack, set 'current hour' to -1
	ADD	R6,R6,#-5	; make space for new entry
	STR	R1,R6,#0	; save string pointer
	LDR	R1,R0,#1	; copy day bitmap
	STR	R1,R6,#1
	LDR	R1,R0,#2	; copy hour bitmap
	STR	R1,R6,#2
	; remaining two locations initialized by FIND_A_SLOT
	AND	R2,R2,#0	; check hour 0 first
	ADD	R1,R2,#1	; bit vector for hour 0 is 1
	; move forward in event list
	ADD	R0,R0,#3

FIND_A_SLOT
	; try to find a slot for the event on top of the stack
	; to enter this code, first set R2 to the next hour to
	; check and R1 to the bit vector for the next hour to check
	LDR	R3,R6,#2	; read hour bitmap
SLOT_POSS
	AND	R4,R3,R1	; try this hour?
	BRnp    FOUND_SLOT
	ADD	R2,R2,#1	; increment hour being examined
	ADD	R1,R1,R1	; shift R1 left
	BRnp	SLOT_POSS	; checked all slots yet?

	; not possible to fit this event, so unwind stack
	LD	R4,ESTACK	; check for BASE
	ADD	R4,R4,R6
	BRnp	TRY_MORE
	; nothing to unwind, so give up
	AND	R0,R0,#0
	BRnzp	AE_DONE
TRY_MORE
	ADD	R6,R6,#5	; remove the failed event
	ADD	R0,R0,#-3	; move backward in event list
IN_CONFLICT
	; need to remove any copies of the event name from the
	; schedule, then try to find a new slot
	LDR	R2,R6,#3	; 'current hour' for event
	ADD	R3,R2,R2	; multiply R2 by 5
	ADD	R3,R3,R3
	ADD	R2,R2,R3 
	LD	R3,SCHED	; R2 points to row for hour being tried
	ADD	R2,R2,R3
	AND	R3,R3,#0	; day count in R3
	LDR	R7,R6,#0	; negative event label
	NOT	R7,R7
	ADD	R7,R7,#1
AE_CONF_DAY
	LDR	R4,R2,#0	; read current slot value
	ADD	R4,R4,R7	; matches event label?
	BRnp	AE_NO_MATCH
	STR	R4,R2,#0	; write back NULL on a match
AE_NO_MATCH
	ADD	R2,R2,#1	; advance to next day slot
	ADD	R3,R3,#1	; increment day count
	ADD	R4,R3,#-5	; done yet?
	BRn	AE_CONF_DAY

	; prepare R1 and R2 for FIND_A_SLOT assumptions
	LDR	R2,R6,#3	; 'current hour' for event
	ADD	R2,R2,#1	; next hour to check
	LDR	R1,R6,#4	; bit vector for 'current hour'
	ADD	R1,R1,R1	; bit vector for next hour
	BRnzp	FIND_A_SLOT

FOUND_SLOT
	; on arrival, R1 = bit vector for slot, R2 = # of slot,
	;             R3 = hour bitmap for event on top of stack
	; found a slot--record attempt in event before trying to insert
	STR	R2,R6,#3	; save 'current hour' to event
	STR	R1,R6,#4	; save 'current hour' bit vector to event
	NOT	R1,R1
	AND	R3,R3,R1	; remove current hour from possible hours
	STR	R3,R6,#2	; save new hour bitmap to event
	; try to insert event on top of stack into slot R2
	ADD	R3,R2,R2	; multiply R2 by 5
	ADD	R3,R3,R3
	ADD	R2,R2,R3 
	LD	R3,SCHED	; R2 points to row for hour being tried
	ADD	R2,R2,R3
	AND	R3,R3,#0	; day count in R3
	ADD	R1,R3,#1	; day bit mask in R1
	LDR	R5,R6,#1	; day bitmap in R5
	LDR	R7,R6,#0	; event label
AE_CHECK_DAY
	AND	R4,R5,R1	; need this day?
	BRz	AE_SKIP_DAY
	LDR	R4,R2,#0	; read current slot value
	BRnp	IN_CONFLICT	; conflict -- remove any entries written 
	STR	R7,R2,#0	; write this event into schedule
AE_SKIP_DAY
	ADD	R2,R2,#1	; advance to next day slot
	ADD	R1,R1,R1	; shift R1 left
	ADD	R3,R3,#1	; increment day count
	ADD	R4,R3,#-5	; done yet?
	BRn	AE_CHECK_DAY

	; success!  try inserting another event
	BRnzp	INSERT_EVENT

AE_DONE	LD	R7,AE_R7	; restore R7
	RET
	
AE_R7	.BLKW	#1
EXTRA	.FILL	x6000
ESTACK	.FILL	x8000


; subroutine to clear the schedule; 
; caller-saved: R0, R1, R5 (and R7, of course)
; others are callee-saved

CLEAR_SCHEDULE
	; start by clearing the schedule, putting NULL into every slot
	; R5 is an iteration counter for this loop
	AND	R0,R0,#0	; put NULL into R0 for storing
	LD	R1,SCHED	; point R1 to the schedule
	LD	R5,SLEN		; set R5 to the number of slots (110)
CLEAR_SCHED
	STR	R0,R1,#0	; write one NULL
	ADD	R1,R1,#1	; point to the next slot
	ADD	R5,R5,#-1	; reduce the count by 1
	BRp	CLEAR_SCHED
	RET

SCHED	.FILL x4000	; location of schedule
SLEN	.FILL #80	; total number of schedule slots


TRANSLATE

;
; translate an event list into a weekly schedule
; (all registers are caller-saved)
; side effects: halts processor on error ... bizarre, but whatever
;

; registers in CP1 translation are used as follows:
;   R0 - pointer to event list
;   R1 - pointer to schedule
;   R2 - pointer to current event name
;   R3 - bitmap of days
;   R4 - count of days to check
;   R5 - bitmask of day to check
;   R6 - NOT USABLE BY THIS CODE
;   R7 - temporary

	ST	R7,TRANS_R7	; save R7
	LD	R0,ELIST	; initialize event list pointer
READ_NEXT_EVENT			; start of event-reading loop
	LD	R1,SCHED	; point R1 to the schedule again

	; check for the end of the event list
	LDR	R6,R0,#0
	BRz	END_OF_EVENTS

	; R0 points to the next event, which starts with the label

	ADD	R2,R0,#0	; save the string pointer in R2
FIND_LABEL_END
	; find the end of the label to find the date and time bitmap values
	ADD	R0,R0,#1
	LDR	R7,R0,#0
	BRnp	FIND_LABEL_END

	; read the rest of the event entry (R0 now points to terminal NUL)
	LDR	R3,R0,#1	; read bitmap of days into R3
	LDR	R4,R0,#2	; read slot number into R4
	ADD	R0,R0,#3	; point R0 to next event

	; check that slot number is valid (0 to 15)
	ADD	R7,R4,#0
	BRn	INVALID_SLOT
	ADD	R7,R4,#-16
	BRn	VALID_SLOT

INVALID_SLOT	; print an error message
	ADD	R0,R2,#0
	PUTS
	LEA	R0,INVALID_SLOT_ERR
	PUTS
	HALT

VALID_SLOT
	; multiply slot number by 5 and add R1 to find 5 days for that time
	ADD	R7,R4,R4
	ADD	R7,R7,R7
	ADD	R4,R4,R7
	ADD	R1,R4,R1

	; R5 is a bitmask of days; R4 counts 5 days
	AND	R5,R5,#0
	ADD	R5,R5,#1
	AND	R4,R4,#0
	ADD	R4,R4,#5

	; check day indicated by bitmap
DAY_LOOP
	AND	R7,R3,R5
	BRz	DAY_NOT_PRESENT
	LDR	R7,R1,#0
	BRz	SLOT_AVAIL

	; slot already taken: print an error and terminate
	ADD	R0,R2,#0	; print event name first
	PUTS
	LEA	R0,SLOT_TAKEN_ERR
	PUTS
	HALT

SLOT_AVAIL
	STR	R2,R1,#0
DAY_NOT_PRESENT
	ADD	R5,R5,R5	; advance to next bit
	ADD	R1,R1,#1	; point to next day in schedule
	ADD	R4,R4,#-1	; one day done
	BRp	DAY_LOOP

	; go get another event
	BRnzp	READ_NEXT_EVENT

END_OF_EVENTS
	LD	R7,TRANS_R7	; restore R7
	RET

TRANS_R7 .BLKW #1
ELIST	.FILL x5000	; location of event list
INVALID_SLOT_ERR	.STRINGZ " has an invalid slot number.\n"
SLOT_TAKEN_ERR		.STRINGZ " conflicts with an earlier event.\n"



PRINT_SCHEDULE

	; now it's time to print the schedule
;
; print the weekly schedule to the display
; (all registers except R5 are caller-saved)
;

; registers in CP1 schedule printing are used as follows:
;   R0 - used for printing characters with OUT
;   R1 - used for printing with PRINT_SLOT and PRINT_CENTERED
;   R2 - column index / day number
;   R3 - pointer to schedule
;   R4 - row index / slot number (hour) 
;   R6 - NOT USABLE BY THIS CODE
;   R7 - temporary

	ST	R7,PS_R7	; save R7
	LEA	R1,BLANK	; leave a blank for the time slot
	JSR	PRINT_CENTERED
	AND	R2,R2,#0	; R2 counts days in the day header
PRINT_DAYS
	LD	R0,VLINE
	OUT
	LEA	R1,DAYNAME
	ADD	R1,R1,R2
	LDR	R1,R1,#0
	JSR	PRINT_CENTERED
	ADD	R2,R2,#1
	ADD	R7,R2,#-5
	BRn	PRINT_DAYS
	LD	R0,NL
	OUT

	LD	R3,SCHED	; R3 points to the schedule
	AND	R4,R4,#0	; R4 is the slot number
PRINT_LOOP
	ADD	R1,R4,#0	; copy R4 to R1
	JSR	PRINT_SLOT
	AND	R2,R2,#0	; R2 counts days
	ADD	R2,R2,#5
PRINT_ONE_LINE
	LD	R0,VLINE
	OUT
	LDR	R1,R3,#0	; read the schedule string
	BRnp	FULL_ENTRY
	LEA	R1,BLANK	; nothing: print a blank
FULL_ENTRY
	JSR	PRINT_CENTERED
	ADD	R3,R3,#1	; point to next schedule day
	ADD	R2,R2,#-1
	BRp	PRINT_ONE_LINE
	LD	R0,NL
	OUT
	ADD	R4,R4,#1
	ADD	R7,R4,#-16
	BRn	PRINT_LOOP

	LD	R7,PS_R7	; restore R7
	RET

PS_R7	.BLKW	#1
DAYNAME .FILL MON
	.FILL TUE
	.FILL WED
	.FILL THU
	.FILL FRI
BLANK	.STRINGZ ""
MON	.STRINGZ "Mon"
TUE	.STRINGZ "Tue"
WED	.STRINGZ "Wed"
THU	.STRINGZ "Thu"
FRI	.STRINGZ "Fri"
NL	.FILL x0A
VLINE	.FILL x7C	; ASCII vertical line character


	

; The PRINT_CENTERED subroutine prints a string centered in six spaces.
; If the string is longer than six characters, only the first six are
; printed.  If the string is shorter, spaces are used to pad the string,
; and the string is centered, with one more trailing space than leading
; space if the string has an odd length.
;
; All registers are callee-saved; only R7 can change.
;
; registers are used as follows:
;   R0 - one character of string / to print
;   R1 - the string to be printed (constant--not changed)
;   R2 - string length / length of spaces for prefix
;   R3 - count of characters to print
;   R5 - temporary
;
PRINT_CENTERED
	ST	R0,PC_R0	; save callee-saved registers
	ST	R2,PC_R2
	ST	R3,PC_R3
	ST	R5,PC_R5
	ST	R7,PC_R7
	AND	R3,R3,#0	; set count of characters to print to 6
	ADD	R3,R3,#6
	; find the string length in R2
	ADD	R5,R1,#0	; point R5 to string
	AND	R2,R2,#0	; set R2 to 0 for length count
PC_LOOP
	LDR	R0,R5,#0	; find end of string
	BRz	FOUND_LEN
	ADD	R5,R5,#1
	ADD	R2,R2,#1
	BRnzp	PC_LOOP
FOUND_LEN
	; if length >= 5, use no leading spaces
	ADD	R5,R2,#-5
	BRzp	NO_LEADING
	; get the number of leading spaces into R2
	LEA	R5,PRETAB	; use length (R2) as index into table
	ADD	R5,R5,R2
	LDR	R2,R5,#0
PREFIX_LOOP
	LD	R0,SPACE	; print R2 spaces
	OUT
	ADD	R3,R3,#-1	; have printed a char, so decrement R3
	ADD	R2,R2,#-1
	BRp	PREFIX_LOOP
NO_LEADING
	ADD	R5,R1,#0	; point R5 to string
STRING_LOOP
	LDR	R0,R5,#0	; print string to end or six chars
	BRz	SUFFIX_LOOP	; end of string, so print trailing spaces
	OUT
	ADD	R5,R5,#1
	ADD	R3,R3,#-1	; have printed a char, so decrement R3
	BRz	PC_DONE		; six characters printed, so all done
	BRnzp	STRING_LOOP
SUFFIX_LOOP
	LD	R0,SPACE	; print trailing spaces to finish six chars
	OUT
	ADD	R3,R3,#-1
	BRp	SUFFIX_LOOP
PC_DONE
	LD	R0,PC_R0	; restore callee-saved registers
	LD	R2,PC_R2
	LD	R3,PC_R3
	LD	R5,PC_R5
	LD	R7,PC_R7
	RET
PC_R0	.BLKW #1	; storage for R0
PC_R2	.BLKW #1	; storage for R2
PC_R3	.BLKW #1	; storage for R3
PC_R5	.BLKW #1	; storage for R5
PC_R7	.BLKW #1	; storage for R7
PRETAB	.FILL x03	; lookup table for number of leading spaces
	.FILL x02
	.FILL x02
	.FILL x01
	.FILL x01


; print a slot number from 0 to 15 as a time from 07:00 to 22:00, 
; followed by a space.  All registers except R7 are callee-saved
; (they do not change).

; R1 is the slot number
; R0 is used for the hour digit

PRINT_SLOT	; slot number is in R1
	ST	R0,PSLT_R0	; save registers to memory
	ST	R1,PSLT_R1
	ST	R7,PSLT_R7
	LD	R0,ZERO
	ADD	R1,R1,#-3
	BRn	HOUR_READY
	ADD	R0,R0,#1
	ADD	R1,R1,#-10
	BRn	HOUR_READY
	ADD	R0,R0,#1
	BRnzp	PRINT_HOUR
HOUR_READY
	ADD	R1,R1,#10
PRINT_HOUR
	OUT
	LD	R0,ZERO
	ADD	R0,R0,R1
	OUT
	LD	R0,COLON
	OUT
	LD	R0,ZERO
	OUT
	OUT
	LD	R0,SPACE
	OUT
	LD	R0,PSLT_R0	 ; restore registers from memory
	LD	R1,PSLT_R1
	LD	R7,PSLT_R7
	RET
PSLT_R0	.BLKW #1	; space for R0 during PRINT_SLOT
PSLT_R1	.BLKW #1	; space for R1 during PRINT_SLOT
PSLT_R7	.BLKW #1	; space for R7 during PRINT_SLOT
ZERO	.FILL x0030	; ASCII digit 0
COLON	.FILL x003A	; ASCII colon character
SPACE	.FILL x0020	; ASCII space character

	.END

