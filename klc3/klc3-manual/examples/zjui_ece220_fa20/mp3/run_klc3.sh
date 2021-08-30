#!/bin/bash

if [ "$#" -ne 1 ]; then
	echo "Usage: run_klc3.sh <test asm file>, using examples/mp3_gold.asm as gold"
  exit 1
fi

if [ ! -f "$1" ]; then
  echo "File \"$1\" not found"
  exit 1
fi

klc3                                              \
--use-forked-solver=false                         \
--copy-additional-file ../../../asserts/replay.sh \
--max-lc3-step-count=200000                       \
--max-lc3-out-length=1100                         \
sched_alloc_.asm                                  \
stack_alloc_.asm                                  \
sched.asm                                         \
extra.asm                                         \
--test "$1"                                       \
--gold examples/mp3_gold.asm

# klc3
# Turn off forked solver
# Copy the replay script
# Limit on step count
# Limit on output length
# Specify 75 memory slots starting from x3800 for student to use (file not copied to student as it ends with "_")
# Specify a memory block for student to use as stack (file not copied to student)
# Symbolic input of normal event list
# Symbolic input of extra event list
# Test code
# Gold code

echo "Note: default output directory is klc3-out-* under the same directory as the last loaded test asm file"