; this is mp1 by Tingkai Liu (3170111148)
; It will include two subroutine
; one called PRINT_SLOT, which will output six characters showing the schedule slot hours
; the other called PRINT_CENTERED, 
; which will output first six characters started at certain memory
; if the characters are less than 6, it will output spaces to reach 6.
		.ORIG	x3000
; PRINT_SLOT goes here

; R1 contains needed time value 0-15, which means 7:00-22:00 and should not be modified
; R2 is used for the calculating the second digit, but the original value will be preserved
; other registers will be preserved for the main program, won't be used

		
PRINT_SLOT	ST R2,STOREA	; store the original value of R2
		ST R7,STOREB	; store R7 for safety
		ADD R2,R1,#-2	; judge whether R1 is bigger than 2
		BRp MORETWO	;
				
		LD R2,ASCZERO	; give R2 the value of '0'
		JSR PRINT_	; output '0' for the first character
		LD R2,ASCSEVEN	; R1 isn't bigger than 2, then the second digit is '7'+R1
		ADD R2,R2,R1	;
		JSR PRINT_	; output R2+R1 as the second character
		BRnzp FOLL	; to output the following

MORETWO		ADD R2,R1,#-12	; bigger than 2, judge whether is bigger than 12
		BRp MORETWEL	;
		LD R2,ASCONE	; output '1' for the first character
		JSR PRINT_	;
		LD R2,ASCSEVEN	; R1 isn't bigger than 12, then the second digit is R1+'7'-10
		ADD R2,R2,R1	;
		ADD R2,R2,#-10	;
		JSR PRINT_	; output R2+R1-10 as the second character
		BRnzp FOLL	; to output the following


MORETWEL	LD R2,ASCTWO	; output the first character '2'
		JSR PRINT_	;
		LD R2,ASCSEVEN	; bigger than 12, the second digit is R1+R2-20
		ADD R2,R2,R1	;
		ADD R2,R2,#-10	;
		ADD R2,R2,#-10	;
		JSR PRINT_	; output the R1+'7'-20 as the second character
		
FOLL		LD R2,ASCM	; output ':00 ', which is the same for all condition
		JSR PRINT_	;		
		LD R2,ASCZERO	;
		JSR PRINT_	;
		JSR PRINT_	;
		LD R2,ASCSPACE	;
		JSR PRINT_	;

		LD R2,STOREA	; give the value of R2 back
		LD R7,STOREB	; give the value of R7 back
		RET



; PRINT_CENTERED goes here

; R1 contains the beginning address of the string which end with x00
; R2 is loading the  characters, and the input of PRINT_
; R3 is used as a counter for leading space, after that it becomes a address pointer
; R4 is used as a counter for trailing space
; R5 is used for getting address
; other registers will be preserved for the main program, won't be used

PRINT_CENTERED	ST R2,STOREA	; store the original value of R2
		ST R3,STOREB	; store the original value of R3
		ST R4,STOREC	; store the original value of R4
		ST R5,STORED	; store the original value of R5
		ST R7,STOREE	; store R7 for safety

		AND R3,R3,#0	; initialize R3 to 0
		AND R4,R4,#0	; initialize R4 to 0
		AND R5,R5,#0	; initialize R5 to 0
		ADD R5,R1,#0	; get the address into R5
		LDR R2,R5,#0	; Test whether the first character is x00
		BRz ZERO	;
		
COUNT		ADD R5,R5,#1	; get the address being test
		ADD R3,R3,#1	; count how many (3-leading space)
		LDR R2,R5,#0	;		
		BRz COUNTED	; 
		ADD R5,R5,#1	; get the next address
		ADD R4,R4,#1	; count how many (3-trailing space)
		LDR R2,R5,#0	; 
		BRz COUNTED	;
		BRnzp COUNT	; 

COUNTED		ADD R5,R3,R4	; temporary store the number of chars
		NOT R4,R4	;
		ADD R4,R4,#1	; get -R4
		ADD R4,R4,#3	; get the number of trailing spaces
		NOT R3,R3	;
		ADD R3,R3,#1	; get -R3
		ADD R3,R3,#3	; get the number of leading spaces
		BRn MSIX	; if the result is negative, it means there are more than 6 chars
		BRz NOLE	; if the result is zero, it means there are 5/6 chars, no leading spaces
		BRnzp LEADING	;

ZERO		ADD R3,R3,#6	;
		AND R5,R5,#0	;

LEADING		LD R2,ASCSPACE	; begin printing leading space
		JSR PRINT_	;
		ADD R3,R3,#-1	;
		BRp LEADING	;
		ADD R5,R5,#0	;
		BRz FINISH	; if no chars to print, finish
		BRnp NOLE	; to print the chars
		
MSIX		AND R5,R5,#0	;
		ADD R5,R5,#6	; need to print exactly 6 chars  

NOLE		ADD R3,R1,#0	; get the beginning address
NEXT		LDR R2,R3,#0	;
		JSR PRINT_	; print that char
		ADD R3,R3,#1	; point to the next address
		ADD R5,R5,#-1	; show how many chars left
		BRp NEXT	;

		ADD R4,R4,#0	;
		BRnz FINISH	; if no spaces to be printed, finish
TRAILING	LD R2,ASCSPACE	;
		JSR PRINT_	;
		ADD R4,R4,#-1	;
		BRp TRAILING	;

FINISH		LD R2,STOREA	; give the value of R2 back
		LD R3,STOREB	; give the value of R3 back
		LD R4,STOREC	; give the value of R4 back
		LD R5,STORED	; give the value of R5 back
		LD R7,STOREE	; give the value of R7 back
		
		RET


; what followed is a sub-subrountine used for output, taking R2 as input of what will be printed
; I write this because I want to try using DDR (but not R0) and using subroutines, haha

PRINT_		ST R5,STOREF	;
PLOOP		LDI R5,DSR	;
		BRzp PLOOP	;
		STI R2,DDR	;
		LD R5,STOREF	;
		RET		



DSR		.FILL xFE04	
DDR		.FILL xFE06
STOREA		.FILL x0000
STOREB		.FILL x0000
STOREC		.FILL x0000
STORED		.FILL x0000
STOREE		.FILL x0000
STOREF		.FILL x0000
ASCZERO		.FILL x0030
ASCONE		.FILL x0031
ASCTWO		.FILL x0032
ASCSEVEN	.FILL x0037
ASCM		.FILL x003A
ASCSPACE	.FILL x0020

.END













