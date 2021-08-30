:boom: **ERROR**: for some input, your program doesn't halt before it reaches the output limit.

**NOTE**: in [test0](test0), at step 19800, program doesn't halt after printing 1101 characters.
<details><summary>The output until your program is killed (click to expand) </summary>

<br>

```
         |  MONDAY | TUESDAY |WEDNESDAY| THURSDAY|  FRIDAY 
   0600  |         |         |         |         |         
   0700  |         |         |         |         |         
   0800  |         |         |         |         |         
   0900  |         |         |         |         |         
   1000  |         |         |         |         |         
   1100  |         |         |         |         |         
   1200  |         |         |         |         |         
   1300  |         |         |         |         |         
   1400  |         |         |         |         |         
   1500  |         |         |         |         |         
   1600  |         |         |         |         |         
   1700  |         |         |         |         |         
   1800  |         |         |         |         |         
   1900  |         |         |         |         |         
   2000  |         |         |         |         |         
   0700  |\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In
   0800  |\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037\xF022\x0FF6\x8000\x8000\x8000
In
   0900  |\xE037\xF022\x0FF6\x8000\x8000\x8000
In|\xE037
```

Spaces may not be shown clearly. Selecting the text may help. ` \xABCD` means that you try to print value 0xABCD, which is not a printable ASCII. 
</details>

**REMARK**: Probably R7 is not restored in your subroutines or there is infinite loop in your code. Notice that your program already prints more characters to the screen than what is expected, which can't be correct. Result comparison is not run until you fix this issue. 

<br><br>

:boom: **ERROR**: incorrect output.

**NOTE**: in [test1](test1), at step 1586, your output (length = 72) is:
```
BBBBBBBBBB conflicts with an earlier event.


--- halting the LC-3 ---


```

The expected output (length = 988) is:
```
         |  MONDAY | TUESDAY |WEDNESDAY| THURSDAY|  FRIDAY 
   0600  |         |         |         |         |         
   0700  |         |         |         |         |         
   0800  |         |         |         |         |         
   0900  |         |         |         |         |         
   1000  |         |         |         |         |         
   1100  |         |         |         |         |         
   1200  |         |         |         |         |         
   1300  |         |         |         |         |         
   1400  |         |         |         |         |         
   1500  |         |         |         |         |         
   1600  |         |         |         |         |         
   1700  |         |         |         |         |         
   1800  |         |         |         |         |         
   1900  |         |         |         |         |         
   2000  |         |    A    |BBBBBBBBB|BBBBBBBBB|BBBBBBBBB


--- halting the LC-3 ---


```

Spaces may not be shown clearly. Selecting the text may help. 


**REMARK**: Your output doesn't match the expected one. 

<br><br>

:warning: **WARNING**: write to uninitialized memory at [[mp2_buggy.asm:68](mp2_buggy.asm#L68)] `STR R7,R5,#0`.

**NOTE**: in [test0](test0), at step 384, writing to unspecified addr x384B.

**REMARK**: This is risky. You are encouraged to use memory that is explicitly specified in your code (.BLKW, .FILL, etc.) or explicitly allowed by the assignment. 

<br><br>

:warning: **WARNING**: use uninitialized register at [[mp2_buggy.asm:387](mp2_buggy.asm#L387)] `LDR R0,R5,#0`.

**NOTE**: in [test0](test0), at step 16343, using uninitialized R5.

**REMARK**: Do not assume the initial value of register. Now it is assumed to be 0 (notice: not necessarily when replay)

<br><br>

:warning: **WARNING**: use uninitialized register at [[mp2_buggy.asm:407](mp2_buggy.asm#L407)] `ADD R4,R4,#1`.

**NOTE**: in [test0](test0), at step 16192, using uninitialized R4.

<br><br>

:warning: **WARNING**: RET to improper location at [[mp2_buggy.asm:420](mp2_buggy.asm#L420)] `RET`.

**NOTE**: in [test0](test0), at step 16191, expect RET to addr x3005 right after [[mp2_buggy.asm:34](mp2_buggy.asm#L34)] `JSR PRINT_SCHEDULE`, but actually RET to addr x30F5.

**REMARK**: Have you correctly restore R7? Or are you skipping levels deliberately? 

<br><br>

:warning: **WARNING**: read a memory location which stores an instruction rather than data at [[mp2_buggy.asm:526](mp2_buggy.asm#L526)] `LDR R0,R1,#0`.

**NOTE**: in [test0](test0), at step 16352, reading addr x044E.

**REMARK**: It's possible that you have calculate the address incorrectly, or maybe you don't preserve enough space for your data. 

<br><br>

:warning: **WARNING**: read a memory location which stores an instruction rather than data at [[mp2_buggy.asm:544](mp2_buggy.asm#L544)] `LDR R0,R1,#0`.

**NOTE**: in [test0](test0), at step 16490, reading addr x044E.

<br><br>

