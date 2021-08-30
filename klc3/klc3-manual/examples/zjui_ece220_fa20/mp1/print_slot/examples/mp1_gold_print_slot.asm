; ================================ KLC3 ENTRY CODE ================================

; This piece of code serves as the entry point to your PRINT_SLOT subroutine.
; The test index is stored at x2700, which is loaded into R1 before calling your subroutine.

.ORIG x3000

LDI R1, KLC3_TEST_INDEX_ADDR
JSR PRINT_SLOT

HALT  ; KLC3: INSTANT_HALT

KLC3_TEST_INDEX_ADDR  .FILL x2700

; ============================= END OF KLC3 ENTRY CODE ============================

; By Steve Lumetta


; The PRINT_CENTERED subroutine prints a string centered in nine spaces.
; If the string is longer than nine characters, only the first nine are
; printed.  If the string is shorter, spaces are used to pad the string,
; and the string is centered, with one more leading space than trailing
; space if the string has an even length.
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
	AND	R3,R3,#0	; set count of characters to print to 9
	ADD	R3,R3,#9
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
	; if length >= 9, use no leading spaces
	ADD	R5,R2,#-9
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
	LDR	R0,R5,#0	; print string to end or nine chars
	BRz	SUFFIX_LOOP	; end of string, so print trailing spaces
	OUT
	ADD	R5,R5,#1
	ADD	R3,R3,#-1	; have printed a char, so decrement R3
	BRz	PC_DONE		; nine characters printed, so all done
	BRnzp	STRING_LOOP
SUFFIX_LOOP
	LD	R0,SPACE	; print trailing spaces to finish nine chars
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
PRETAB	.FILL x05	; lookup table for number of leading spaces
	.FILL x04
	.FILL x04
	.FILL x03
	.FILL x03
	.FILL x02
	.FILL x02
	.FILL x01
	.FILL x01


; print a slot number from 0 to 14 as a time from 0600 to 2000,
; preceded and followed by a space.  All registers except R7 are
; callee-saved (they do not change).

; R1 is the slot number
; R0 is used for the hour digit

PRINT_SLOT	; slot number is in R1
	ST	R0,PSLT_R0	; save registers to memory
	ST	R1,PSLT_R1
	ST	R7,PSLT_R7
	LD	R0,SPACE
	OUT
	OUT
	OUT
	LD	R0,ZERO
	ADD	R1,R1,#-4
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
	LD	R0,ZERO
	OUT
	OUT
	LD	R0,SPACE
	OUT
	OUT
	LD	R0,PSLT_R0	 ; restore registers from memory
	LD	R1,PSLT_R1
	LD	R7,PSLT_R7
	RET
PSLT_R0	.BLKW #1	; space for R0 during PRINT_SLOT
PSLT_R1	.BLKW #1	; space for R1 during PRINT_SLOT
PSLT_R7	.BLKW #1	; space for R7 during PRINT_SLOT
ZERO	.FILL x0030	; ASCII digit 0
SPACE	.FILL x0020	; ASCII space character

	.END
