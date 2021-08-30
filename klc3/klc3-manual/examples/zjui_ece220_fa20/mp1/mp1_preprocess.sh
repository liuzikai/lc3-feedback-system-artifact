#!/bin/bash

# This script combines mp1.asm with entry code and generates mp1_print_centered.asm and mp1_print_slot.asm

SUBROUTINES=("print_centered" "print_slot")

for subroutine in "${SUBROUTINES[@]}"; do

  out_filename="mp1_${subroutine}.asm"

  cp "${subroutine}/entry.asm" "${out_filename}"

  sed -e '/.ORIG/d' -e '/.END/d' "mp1.asm" >> "${out_filename}"

  printf "\n.END\n" >> "${out_filename}"
  
done