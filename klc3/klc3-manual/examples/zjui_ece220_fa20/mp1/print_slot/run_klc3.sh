#!/bin/bash

if [ "$#" -ne 1 ]; then
	echo "Usage: run_klc3.sh <test asm file>, using examples/mp1_gold_print_slot.asm as gold"
  exit 1
fi

if [ ! -f "$1" ]; then
  echo "File \"$1\" not found"
  exit 1
fi

klc3                                                  \
--use-forked-solver=false                             \
--copy-additional-file ../../../../asserts/replay.sh  \
--max-lc3-step-count=50000                            \
--max-lc3-out-length=50                               \
--report-replace-space                                \
../assertions_.asm                                    \
data.asm                                              \
--test "$1"                                           \
--gold examples/mp1_gold_print_slot.asm

# klc3
# Turn off forked solver
# Copy the replay script
# Limit on step count
# Limit on output length
# Replace spaces with "_" for LC-3 output in report
# Customized issues and assertions (file not copied to student as it ends with "_")
# Symbolic input space
# Test code combined with entry.asm
# Gold code combined with entry.asm

echo "Note: default output directory is klc3-out-* under the same directory as the last loaded test asm file"
