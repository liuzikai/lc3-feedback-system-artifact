; This is mp2 by Tingkai Liu (3170111148)
; it will contain a program that
; translate the event list from x5000 
; into a schedule start at x4000 and print it out
; each event in the event list include 3 fields
; NUL-end ASCII label, day of the week by bit vector, 
; and hour slot (0-15 declare 7-22 o'clock)
; the schedule is a 16*5 table, showing every 16 hours in five days
; except PRINT_SLOT and PRINT_CENTERED
; it also have some additional subroutines
; TEST_DAY is used to get which day in the week is occupied and store the condition in certain memory
; (follows not real subroutines because it will end the whole program and use BR)
; SLOT_ERR is used to output the message of invalid slot number
; EVENT_ERR is used to output the message of conflict event

; R0 is used for output and temporary address pointer
; R1 is used as address pointer in event table and output
; R2 is used for getting the data in the address and temporary
; R3 is a counter
; R4 is used for getting days (second field) in getting event, counter for days in output
; R5 is used for getting hours (third field) in getting event, counter for hours in output
; R6 is used for pointer in schedule
; R7 is saved


.ORIG		x3000
		LD R3,SIZE	; initialize R3 to hold the size of the schedule 16*5=80=x50
		LD R6,SCHEDULE	; get the begin address x4000 of the schedule into R6
		LD R2,ASCNUL	; get x00 into R2 for write

SCHELOOP1	STR R2,R6,#0	; initialize the schedule at x4000 (16*5)
		ADD R6,R6,#1	;
		ADD R3,R3,#-1	;
		BRp SCHELOOP1	;

		LD R1,EVENT	; begin to get the event in x5000 and store it schedule
GETEVENT	ADD R0,R1,#0	; point the the address of the event string
GETDAY		LDR R2,R1,#0	; get the second field (day)
		BRz GOTDAY	;
		ADD R1,R1,#1	;
		BRnzp GETDAY	;
GOTDAY		LDR R4,R1,#1	; get the day into R4
		LDR R5,R1,#2	; get the third field (hour) into R5
		BRn SLOT_ERR	; test whether the hour is valid
		ADD R2,R5,#-15	;
		BRp SLOT_ERR	; if not, output error and stop
		JSR TEST_DAY	;
		LD R6,SCHEDULE	; calculate the address in schedule x4000+hour*5+day
		LD R3,WU	; counter for multiply
MULTIPLY	ADD R6,R6,R5	;
		ADD R3,R3,#-1	;
		BRp MULTIPLY	;
		LD R3,WU	; count for five days
		LEA R4,ISMON	; temporary use R4 for address pointer for testing the days
		ADD R4,R4,#-1	; (same as the next command)
		ADD R6,R6,#-1	; to suit the following loop, decrease the pointer and it will be added back afterwards
MANYDAYS	ADD R4,R4,#1	;
		ADD R3,R3,#-1	; test which day is arranged
		BRn GNE		; stop after testing five days
		ADD R6,R6,#1	;
		LDR R2,R4,#0	;
		BRz MANYDAYS	; if that day is not arranged, test the next day
		LDR R2,R6,#0	; if it is arranged, test whether there is a event already
		BRnp EVENT_ERR	; if yes, output error end stop
		STR R0,R6,#0	; store the address of the event first char
		BRnzp MANYDAYS	; next day

GNE		ADD R1,R1,#3	; get next event
		LDR R2,R1,#0	;
		BRnp GETEVENT	; if it is not x00, means the events didn't finish


; begin to output the schedule
		LEA R1,ASCNUL	; store the address to be printed in R1
		JSR PRINT_CENTERED ; print 6 spaces at the front
		LD R0,ASCLINE	;
		OUT
		LD R3,WU	; print the headline with days
		LEA R1,MON	;
DAYS		JSR PRINT_CENTERED ;		
		ADD R1,R1,#4	;
		ADD R3,R3,#-1	;
		BRz HEADDONE	;
		LD R0,ASCLINE	;
		OUT		;
		BRnzp DAYS	;
HEADDONE	LD R0,ASCFEED	;
		OUT		;
		LD R3,SIZE	; init R3 for counting the size
		LD R6,SCHEDULE	; address pointer of the schedule
		AND R5,R5,#0	; init R5 for counting the hours

P_NEXTH		ADD R1,R5,#0	; store the hour in R1
		JSR PRINT_SLOT	; print each line leading with hour
		ADD R5,R5,#1	; point to the next hour
		LD R0,ASCLINE	;
		OUT		;
		AND R4,R4,#0	;
		ADD R4,R4,#5	; event counter
NEXTE		LDR R1,R6,#0	; get the pointer into R1
		BRz NO_EVENT	;
NO_E_BACK	JSR PRINT_CENTERED ; print the event
		ADD R6,R6,#1	; next event
		ADD R3,R3,#-1	; decrease the counter for the whole table
		ADD R4,R4,#-1	; decrease the counter for the events in this hour
		BRz NEXTH
		LD R0,ASCLINE	; print vertical line except for the last event
		OUT		;		
		BRnzp NEXTE	;
NEXTH		LD R0,ASCFEED	;
		OUT		; change line at the end of each line
		ADD R3,R3,#0	; when five events have printed, next hour if didn't finish
		BRp P_NEXTH	;

SCHEFINISH	HALT
		

NO_EVENT	LEA R1,ASCNUL	; store the address of nul for printing 6 spaces
		BRnzp NO_E_BACK ;


SLOT_ERR	PUTS
		LEA R0,SLOT_ERRM		; output " has an invalid slot number.\n"
		PUTS
		BRnzp SCHEFINISH


EVENT_ERR	PUTS
		LEA R0,EVENT_ERRM		; output " conflicts with an earlier event.\n"
		PUTS
		BRnzp SCHEFINISH


; R4 is the input as a bit vector for TEST_DAY
TEST_DAY	ST R0,STOREA	; for testing the day
		ST R1,STOREB	; address pointer
		ST R2,STOREC	; counter
		ST R6,STORED	; for temporary use
		AND R0,R0,#0	; init
		AND R6,R6,#0	; asc x00
		LEA R1,ISMON	;
		LD R2,WU	; count five days
CLEARLOOP	STR R6,R1,#0	;
		ADD R1,R1,#1	;
		ADD R2,R2,#-1	;
		BRp CLEARLOOP	;

		ADD R0,R0,#1	; Monday bit
		LD R2,WU	; count five days
		LEA R1,ISMON	;
TESTLOOP	AND R6,R4,R0	;
		STR R6,R1,#0	; 0 means not that day, others numbers means yes
		ADD R0,R0,R0	; next day bit
		ADD R1,R1,#1	; point to the next day condition bit
		ADD R2,R2,#-1	; decrease the counter
		BRp TESTLOOP	;
		LD R0,STOREA	;
		LD R1,STOREB	;
		LD R2,STOREC	;
		LD R6,STORED	;
		RET

		
		
		

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




ASCNUL		.FILL x0000
SCHEDULE	.FILL x4000
EVENT		.FILL x5000
SIZE		.FILL x0050
ASCLINE		.FILL x007C
ASCFEED		.FILL x000A
ISMON		.FILL x0000
ISTUE		.FILL x0000
ISWED		.FILL x0000
ISTHU		.FILL x0000
ISFRI		.FILL x0000
WU		.FILL x0005

MON		.FILL x004D
		.FILL x006F
		.FILL x006E
		.FILL x0000
TUE		.FILL x0054
		.FILL x0075
		.FILL x0065
		.FILL x0000
WED		.FILL x0057
		.FILL x0065
		.FILL x0064
		.FILL x0000
THU		.FILL x0054
		.FILL x0068
		.FILL x0075
		.FILL x0000
FRI		.FILL x0046
		.FILL x0072
		.FILL x0069
		.FILL x0000
SLOT_ERRM	.FILL x0020
		.FILL x0068
		.FILL x0061
		.FILL x0073
		.FILL x0020
		.FILL x0061
		.FILL x006E
		.FILL x0020
		.FILL x0069
		.FILL x006E
		.FILL x0076
		.FILL x0061
		.FILL x006C
		.FILL x0069
		.FILL x0064
		.FILL x0020
		.FILL x0073
		.FILL x006C
		.FILL x006F
		.FILL x0074
		.FILL x0020
		.FILL x006E
		.FILL x0075
		.FILL x006D
		.FILL x0062
		.FILL x0065
		.FILL x0072
		.FILL x002E
		.FILL x000A
		.FILL x0000
EVENT_ERRM	.FILL x0020
		.FILL x0063
		.FILL x006F
		.FILL x006E
		.FILL x0066
		.FILL x006C
		.FILL x0069
		.FILL x0063
		.FILL x0074
		.FILL x0073
		.FILL x0020
		.FILL x0077
		.FILL x0069
		.FILL x0074
		.FILL x0068
		.FILL x0020
		.FILL x0061
		.FILL x006E
		.FILL x0020
		.FILL x0065
		.FILL x0061
		.FILL x0072
		.FILL x006C
		.FILL x0069
		.FILL x0065
		.FILL x0072
		.FILL x0020
		.FILL x0065
		.FILL x0076
		.FILL x0065
		.FILL x006E
		.FILL x0074
		.FILL x002E
		.FILL x000A
		.FILL x0000

.END


		