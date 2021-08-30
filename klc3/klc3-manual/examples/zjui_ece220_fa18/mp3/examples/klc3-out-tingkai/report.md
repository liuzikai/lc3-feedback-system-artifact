:boom: **ERROR**: incorrect output.

**NOTE**: in [test0](test0), at step 2031, your output (length = 68) is:
```
Could not fit all events into schedule.


--- halting the LC-3 ---


```

The expected output (length = 742) is:
```
      | Mon  | Tue  | Wed  | Thu  | Fri  
07:00 |      |      |      |      |      
08:00 |      |      |      |      |      
09:00 |      |      |      |      |      
10:00 |      |      |      |      |      
11:00 |      |      |      |      |      
12:00 |      |      |      |      |      
13:00 |      |      |      |      |      
14:00 |      |      |      |      |      
15:00 |      |      |      |      |      
16:00 |      |      |      |      |      
17:00 |      |      |      |      |      
18:00 |      |      |      |      |      
19:00 |      |      |      |      |      
20:00 |      |      |      |      |      
21:00 |      |      |      |      |      
22:00 |      |      |      |      |      


--- halting the LC-3 ---


```

Spaces may not be shown clearly. Selecting the text may help. 


**REMARK**: Your output doesn't match the expected one. 

<br><br>

:warning: **WARNING**: write to uninitialized memory at [[mp3_tingkai.asm:47](mp3_tingkai.asm#L47)] `STR R5,R2,#0`.

**NOTE**: in [test1](test1), at step 9, writing to unspecified addr x7000.

**REMARK**: This is risky. You are encouraged to use memory that is explicitly specified in your code (.BLKW, .FILL, etc.) or explicitly allowed by the assignment. 

<br><br>

:warning: **WARNING**: read a memory address outside the expected regions at [[mp3_tingkai.asm:79](mp3_tingkai.asm#L79)] `LDR R3,R0,#0`.

**NOTE**: in [test2](test2), at step 1035, reading addr x7003.

**REMARK**: Remember that uninitialized memory locations have undefined value. Make sure you have calculated address correctly. If it is the location you want to read, make sure you have initialized it. 

<br><br>

:warning: **WARNING**: read a memory address outside the expected regions at [[mp3_tingkai.asm:149](mp3_tingkai.asm#L149)] `LDR R5,R2,#0`.

**NOTE**: in [test0](test0), at step 686, reading addr x7002.

<br><br>

:warning: **WARNING**: detect a code block shared by subroutine(s) and/or main code at [[mp3_tingkai.asm:451](mp3_tingkai.asm#L451)] `LD R0,OUTSTA`.

**NOTE**: Shared by: INIT_SCHE & OUT_SCHE.
**REMARK**: This issue is raised possibly because you reuse a piece of code by multiple subroutines and/or the "main" (entry) code, using BR or JMP. It may work in LC-3 but you are not encouraged to do so. If you want to reuse some code, make a subroutine. Another possibility is that you RET incorrectly (not using RET but JMP or BR, or doesn't correctly restore R7). 

<br><br>

:boom: **ERROR**: execute data as an instruction at [[mp3_tingkai.asm:459](mp3_tingkai.asm#L459)] `RET`.

**NOTE**: in [test3](test3), at step 701, trying to execute addr x0000.

**REMARK**: Do you forget to restore R7 before RET? 

<br><br>

