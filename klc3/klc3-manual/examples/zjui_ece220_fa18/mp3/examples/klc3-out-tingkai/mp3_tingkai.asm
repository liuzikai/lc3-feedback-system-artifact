; This is mp3 by Tingkai Liu (3170111148)
; it will be based on mp2 (includes mp1) written by MYSELF. ALL RIGHT RESERVED
; in this program, 
; it will find a best way to insert all extra event
; in a schedule that already have something inside 
; with DFS by stack
; and print the new schedule out
; if it is impossible to insert all extra events
; error message will be output instead
; the extra event begins at address x6000
; each event has three field
; first is a string pointer for the address describing the event
; second is a bit vector for days (bit 0-4 indicate MON-FRI)
; third is a bit vector for POSSIBLE hours (bit 0-15 indicate 7 to 22 o'clock)
; The point of DFS is to find a suitable combination of hours for all events.
; the base of the stack is x8000
; The stack will stores the present slot number (0-15) of the event
; mp2 will be changed into subroutines for init schedule, print, etc.

; IS_OK is a subroutine to check if it is ok to insert event with present slot number, output the condition by memory
; TEST_SLOT is a subroutine to check which slot is possible and store it in memory, or -1 if no possibilities
; and delete the used possibility
; STR_EVENT is a subroutine for storing the present possible event into the schedule

; R0 is a address pointer for first field (event address pointer)
; R1 is a address pointer for second field (bit vector for days)
; R2 is a address pointer for third field (bit vector for possible hours)
; R3 is for temporary 
; R4 is for temporary
; R5 is for the items in memory
; R6 is a stack pointer
; R7 is reserved for function calling



.ORIG		x3000
		
		LD R1,EXT_EVENT	; get the beginning address of extra event(x6000)
		LD R2,COPY_EXT	; point to x7000 because I want to copy the extra events here so that I can make some modification
		NOT R5,R2	;
		ADD R5,R5,#1	; get -R2
		ADD R5,R1,R5	; get the difference between two list, with which I can easily get across lists
		ST R5,DIFF_LIST ; store this difference in memory for use

COPYEVENT 	LDR R5,R1,#0	;
		BRz COPYDONE	;
		STR R5,R2,#0	;
		ADD R1,R1,#1	;
		ADD R2,R2,#1	;
		BRnzp COPYEVENT	;

COPYDONE	JSR INIT_SCHE	; initialize the schedule with the present event

; begin trying to insert event with DFS
		LD R6,STACK_BASE ; point R6 to the stack base.
		AND R5,R5,#0	;
		ADD R5,R5,#-1	;
		STR R5,R6,#0	; let the base of the stack =-1
		LD R0,COPY_EXT	; point to the address of the first field
		

NEXT_EX_EVENT	ADD R6,R6,#-1	; decrease the stack pointer
		AND R5,R5,#0	;
		ADD R5,R5,#8	;
		ADD R5,R5,#8	;		
		STR R5,R6,#0	; store 16 in the stack	as init, which is beyond the biggest possible slot number	
GET_FIELD	ADD R1,R0,#1	; get address of the second field (bit vector for days)
		ADD R2,R0,#2	; get address of the third field (bit vector for possible hours)

NEXT_POS	JSR TEST_SLOT	;		
		LD R5,POS_SLOT	; test whether it has possibilities
		BRn GO_BACK	; if not, go back
		JSR IS_OK	; test if the present possible slot is OK
		LD R4,INSERT_OK ;
		BRz NEXT_POS	; if not, test the next possible slot
		STR R5,R6,#0	; if yes, store this event in the schedule. store the present possible slot in the stack
		JSR STR_EVENT	; 
		ADD R0,R0,#3	; the address of the first field of the next event
		LDR R3,R0,#0	; test whether it is done with extra event
		BRz F_SCHE	; if yes, finish the schedule
		BRnp NEXT_EX_EVENT ; if not, get the next event
				
GO_BACK		LDR R3,R6,#1	; test whether it is the first event
		BRn ERR_MP3	; if the first event has to go back, means fail
		ADD R1,R0,#1	; get address of the second field (bit vector for days)
		ADD R2,R0,#2	; get address of the third field (bit vector for possible hours)
		LDR R5,R6,#0	; get the current slot number
		ST R5,POS_SLOT	;

		ADD R5,R5,#-15	; test whether it is 16 (the event haven't been inserted)
		BRp POPOUT	; if yes, means I don't need to clear the schedule, and I can pop directly

		LDR R5,R6,#0	; get the current slot in R5
		LDR R3,R0,#0	; R3 points to the address of the event
		ST R3,CUR_EVENT ; store the address R0 points to 
		AND R3,R3,#0	;
		STR R3,R0,#0	; clear the event being inserted
		JSR STR_EVENT	;
		LD R3,CUR_EVENT	;
		STR R3,R0,#0	; give the event address back

POPOUT		LD R3,DIFF_LIST ;
		ADD R4,R2,R3	; point to the second field of the original list
		LDR R4,R4,#0	; get the original possible slot
		STR R4,R2,#0	; store the possible slot back in the copy list
		ADD R6,R6,#1	; pop the event out of the stack
		ADD R0,R0,#-3	; point to the previous event

		LDR R5,R6,#0	; get the current slot number
		ADD R5,R5,#-15	; test whether it is 16 (the event haven't been inserted)
		BRp GET_FIELD	;
		LDR R5,R6,#0	; get the current slot in R5
		LDR R3,R0,#0	; R3 points to the address of the event
		ST R3,CUR_EVENT ; store the address R0 points to 
		AND R3,R3,#0	;
		STR R3,R0,#0	; clear the event being inserted
		JSR STR_EVENT	;
		LD R3,CUR_EVENT	;
		STR R3,R0,#0	; give the event address back
		 

		BRnzp GET_FIELD	;

F_SCHE		JSR OUT_SCHE	; print out the schedule
MP3_DONE	HALT


ERR_MP3		LEA R0,MP3_ERR_MES ;
		PUTS
		BRnzp MP3_DONE	;

STACK_BASE	.FILL x8000
EXT_EVENT	.FILL x6000
COPY_EXT	.FILL x7000
DIFF_LIST	.FILL x0000
CUR_EVENT	.FILL x0000
MP3_ERR_MES	.STRINGZ "Could not fit all events into schedule.\n"

; TEST_SLOT goes here
; R2 is the input, address of slot bit vector
; R6 is the input for the stack pointer pointing to the present slot
TEST_SLOT	ST R0,TSSTOREA	; stores the registers 
		ST R1,TSSTOREB	;
		ST R3,TSSTOREC	;
		ST R4,TSSTORED	;
		ST R5,TSSTOREE	;

		AND R3,R3,#0	; test from the slot 0 (07:00)
		LDR R5,R2,#0	; get the slot bit vector in R5
		LD R4,TESTER	; get x0001 into R4 for testing the bit vector
		ADD R0,R3,#0	; get a copy of the present slot in R0
TESTERLOOP	BRz TESTERDONE	;
		ADD R4,R4,R4	;
		ADD R3,R3,#-1	;
		BRnzp TESTERLOOP ;
TESTERDONE	AND R1,R5,R4	; test whether the current slot is possible
		BRz NEXT_POSS	; if it is not possible, try the next slot
		NOT R4,R4	;
		ADD R4,R4,#1	; get -R4 into R4
		ADD R5,R5,R4	; delete the possibility being used
		STR R5,R2,#0	;
		ST R0,POS_SLOT	; store the possible slot 
		BRnzp POS_SLOT_G ;
 
NEXT_POSS	ADD R3,R0,#1	; next slot number
		ADD R4,R3,#-15	; test whether it is already 15
		BRz NO_POSS	;
		LD R4,TESTER	; R4=1
		ADD R0,R3,#0	; get the copy of the present slot
		BRnzp TESTERLOOP ;

NO_POSS		AND R3,R3,#0	; store -1 into memory is no possibility remianed
		ADD R3,R3,#-1	;
		ST R3,POS_SLOT	;
		
POS_SLOT_G	LD R0,TSSTOREA	; load the registers back
		LD R1,TSSTOREB	;
		LD R3,TSSTOREC	;
		LD R4,TSSTORED	;
		LD R5,TSSTOREE	;

		RET


POS_SLOT	.FILL x0000
TESTER		.FILL x0001
TSSTOREA	.FILL x0000
TSSTOREB	.FILL x0000
TSSTOREC	.FILL x0000
TSSTORED	.FILL x0000
TSSTOREE	.FILL x0000


; IS_OK goes here
; TRY_INSET is a subroutine for it because I don't want to modify the registers from mp2
; R1 contains the address of the second field (days)
IS_OK		ST R7,IOSTOREA	;
		ST R4,IOSTOREB	;
		ST R5,IOSTOREC	;

		LDR R4,R1,#0	; get the days in R4
		LD R5,POS_SLOT	; get the slot in R5
		JSR TRY_INSERT	;

		LD R7,IOSTOREA	;
		LD R4,IOSTOREB	;
		LD R5,IOSTOREC	;
		RET

IOSTOREA	.FILL x0000
IOSTOREB	.FILL x0000
IOSTOREC	.FILL x0000

; input: R4 contains the days vector, R5 contains the slot number
TRY_INSERT	ST R3,TISTOREA	; stores the registers
		ST R4,TISTOREB	;
		ST R6,TISTOREC	;
		ST R7,TISTORED	;
		ST R2,TISTOREE	;
		AND R3,R3,#0	;
		ADD R3,R3,#1	; pre-store the OK condition
		ST R3,INSERT_OK ;
	
		JSR TEST_DAY	;
		LD R6,SCHEDULE	; calculate the address in schedule x4000+hour*5+day
		LD R3,WU	; counter for multiply
MULTIPLY2	ADD R6,R6,R5	;
		ADD R3,R3,#-1	;
		BRp MULTIPLY2	;
		LD R3,WU	; count for five days
		LEA R4,ISMON	; R4 for address pointer for testing the days
		ADD R4,R4,#-1	; (same as the next command)
		ADD R6,R6,#-1	; to suit the following loop, decrease the pointer and it will be added back afterwards
MANYDAYS2	ADD R4,R4,#1	;
		ADD R3,R3,#-1	; test which day is arranged
		BRn TEST_F	; stop after testing five days
		ADD R6,R6,#1	;
		LDR R2,R4,#0	;
		BRz MANYDAYS2	; if that day is not arranged, test the next day
		LDR R2,R6,#0	; if it is arranged, test whether there is a event already
		BRnp EVENT_ERR2	; if yes, output error and stop
		BRnzp MANYDAYS2	; next day

EVENT_ERR2	AND R3,R3,#0	;
		ST R3,INSERT_OK	;

TEST_F		LD R3,TISTOREA	; load the registers 
		LD R4,TISTOREB	;
		LD R6,TISTOREC	;
		LD R7,TISTORED	;
		LD R2,TISTOREE	;
		RET

INSERT_OK	.FILL x0000
TISTOREA	.FILL x0000
TISTOREB	.FILL x0000
TISTOREC	.FILL x0000
TISTORED	.FILL x0000
TISTOREE	.FILL x0000

; STR_EVENT goes here
; R0 contains the address of the first field (event name address)
; R1 contains the address of the second field (days)
; R5 contains the slot number

STR_EVENT	ST R4,IOSTOREA	;
		ST R7,IOSTOREB	;
		ST R0,IOSTOREC	;

		LDR R0,R0,#0	;
		LDR R4,R1,#0	; get the days in R4
		JSR MP2STR	;

		LD R4,IOSTOREA	;
		LD R7,IOSTOREB	;
		LD R0,IOSTOREC
		RET

; (from mp2)		
MP2STR		ST R3,TISTOREA	; stores the registers
		ST R4,TISTOREB	;
		ST R6,TISTOREC	;
		ST R7,TISTORED	;
		ST R2,TISTOREE	;

		JSR TEST_DAY	;
		LD R6,SCHEDULE	; calculate the address in schedule x4000+hour*5+day
		LD R3,WU	; counter for multiply
MULTIPLY3	ADD R6,R6,R5	;
		ADD R3,R3,#-1	;
		BRp MULTIPLY3	;
		LD R3,WU	; count for five days
		LEA R4,ISMON	; temporary use R4 for address pointer for testing the days
		ADD R4,R4,#-1	; (same as the next command)
		ADD R6,R6,#-1	; to suit the following loop, decrease the pointer and it will be added back afterwards
MANYDAYS3	ADD R4,R4,#1	;
		ADD R3,R3,#-1	; test which day is arranged
		BRn NEW_STED	; stop after testing five days
		ADD R6,R6,#1	;
		LDR R2,R4,#0	;
		BRz MANYDAYS3	; if that day is not arranged, test the next day
		STR R0,R6,#0	; store the address of the event first char
		BRnzp MANYDAYS3	; next day

NEW_STED	LD R3,TISTOREA	; load the registers 
		LD R4,TISTOREB	;
		LD R6,TISTOREC	;
		LD R7,TISTORED	;
		LD R2,TISTOREE	;
		RET


; the following is the copy of mp2 (which contains mp1) 
; it is changed into subroutines
; INIT_SCHE is for getting events from x5000 to schedule
; OUT_SCHE is for printing the schedule


ASCNUL		.FILL x0000
SCHEDULE	.FILL x4000
EVENT		.FILL x5000
SIZE		.FILL x0050

INIT_SCHE	ST R0,INITSTA	; stores the registers 
		ST R1,INITSTB	;
		ST R2,INITSTC	;
		ST R3,INITSTD	;
		ST R4,INITSTE	;
		ST R5,INITSTF	;
		ST R6,INITSTG	;
		ST R7,INITSTH	;

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
		
		LD R0,INITSTA	; load the registers back
		LD R1,INITSTB	;
		LD R2,INITSTC	;
		LD R3,INITSTD	;
		LD R4,INITSTE	;
		LD R5,INITSTF	;
		LD R6,INITSTG	;
		LD R7,INITSTH	;
		RET

; memories for storing registers
INITSTA		.FILL x0000
INITSTB		.FILL x0000
INITSTC		.FILL x0000
INITSTD		.FILL x0000
INITSTE		.FILL x0000
INITSTF		.FILL x0000
INITSTG		.FILL x0000
INITSTH		.FILL x0000


; here comes OUT_SCHE

OUT_SCHE	ST R0,OUTSTA	; stores the registers 
		ST R1,OUTSTB	;
		ST R2,OUTSTC	;
		ST R3,OUTSTD	;
		ST R4,OUTSTE	;
		ST R5,OUTSTF	;
		ST R6,OUTSTG	;
		ST R7,OUTSTH	;

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

SCHEFINISH	LD R0,OUTSTA	; load the registers back
		LD R1,OUTSTB	;
		LD R2,OUTSTC	;
		LD R3,OUTSTD	;
		LD R4,OUTSTE	;
		LD R5,OUTSTF	;
		LD R6,OUTSTG	;
		LD R7,OUTSTH	;
		RET

; memories for storing registers
OUTSTA		.FILL x0000
OUTSTB		.FILL x0000
OUTSTC		.FILL x0000
OUTSTD		.FILL x0000
OUTSTE		.FILL x0000
OUTSTF		.FILL x0000
OUTSTG		.FILL x0000
OUTSTH		.FILL x0000


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
; TEST_DAY is used to get which day in the week is occupied and store the condition in certain memory
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
		ST R2,STOREG	;
PLOOP		LDI R5,DSR	;
		BRzp PLOOP	;
		STI R2,DDR	;

		LD R5,STOREF	;
		LD R2,STOREG	;
		RET		



DSR		.FILL xFE04	
DDR		.FILL xFE06
STOREA		.FILL x0000
STOREB		.FILL x0000
STOREC		.FILL x0000
STORED		.FILL x0000
STOREE		.FILL x0000
STOREF		.FILL x0000
STOREG		.FILL x0000
ASCZERO		.FILL x0030
ASCONE		.FILL x0031
ASCTWO		.FILL x0032
ASCSEVEN	.FILL x0037
ASCM		.FILL x003A
ASCSPACE	.FILL x0020



.END






