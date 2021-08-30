:warning: **WARNING**: use uninitialized register at [[get_sign.asm:10](get_sign.asm#L10)] `ADD R0, R0, #1`.

**NOTE**: in [test0](test0), at step 4, using uninitialized R0.

**REMARK**: Do not assume the initial value of register. Now it is assumed to be 0 (notice: not necessarily when replay)

<br><br>

:boom: **ERROR**: RET in main code at [[get_sign.asm:20](get_sign.asm#L20)] `RET`.

**NOTE**: in [test1](test1), at step 5, main code starts at: x3000.

**REMARK**: Do not RET in main code. If you need to exit the program, use HALT. 

<br><br>

