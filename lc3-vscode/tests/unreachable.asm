.ORIG x3000

; TEST: Unreachable instruction
BR SKIP_INSTRUCTION
ADD R0, R0, #0
SKIP_INSTRUCTION
ADD R0, R0, #0

.END