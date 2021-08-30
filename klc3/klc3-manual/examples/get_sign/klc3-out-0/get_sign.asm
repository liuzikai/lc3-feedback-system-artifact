; A BUGGY program that sets R0 to -1, 0 or 1 based on the sign of the input number stored at memory x4000.
; liuzikai 2020.04.30

.ORIG x3000             ; program starts at x3000

LDI R1, DATA_ADDR       ; load the input number into R1
BRn NEGATIVE_CASE
BRz ZERO_CASE
POSITIVE_CASE
    ADD R0, R0, #1      ; BUG: use uninitialized register R0
    HALT
ZERO_CASE
    AND R0, R0, #0      ; set R0 to 0
                        ; although R0 is used as the operand, this is not a bug
                        ; as the result is always 0 regardless of R0
    HALT
NEGATIVE_CASE
    AND R0, R0, #0
    ADD R0, R0, #-1
    RET                 ; BUG: RET in main code (should use HALT)

DATA_ADDR .FILL x4000  ; address where the input number stores

.END