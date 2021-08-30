#!/bin/bash

# Check for argument
if [ "$#" -ne 1 ] || [ -n "${1//[0-9]/}" ]; then
  echo "Usage: $0 <test index>" >&2
  exit 1
fi

basename="test$1"
# Check for test directory
if ! [ -d "${basename}" ]; then
  echo "Fail to find test case \"${basename}\"."
  exit 1
fi

# Check for lcs script file
if ! [ -f "${basename}/${basename}.lcs" ]; then
  echo "No replay script found."
  exit 1
fi

# Check for lc3as and lc3sim
if ! command -v lc3as &>/dev/null; then
  echo "lc3as is not available. Make sure you have install lc3tools correctly."
fi
if ! command -v lc3sim &>/dev/null; then
  echo "lc3as is not available. Make sure you have install lc3tools correctly."
fi

# Compile all asm files
for filename in *.asm; do
  if ! lc3as "$filename" >/dev/null; then
    echo "Fail to compile test data ${filename}. Make sure you have installed lc3tools correctly. Otherwise, please contact TAs."
    exit 2
  fi
done
for filename in ${basename}/*.asm; do
  if ! lc3as "$filename" >/dev/null; then
    echo "Fail to compile test data ${filename}. Make sure you have installed lc3tools correctly. Otherwise, please contact TAs."
    exit 2
  fi
done

# Check for new version of lc3sim
if lc3sim -h | grep -q "\-S <script file>"; then
  lc3sim -S "${basename}/${basename}.lcs"
else
  # Use expect if available
  if command -v expect &>/dev/null; then
    expect -c "spawn lc3sim; expect \"(lc3sim) \"; send \"execute ${basename}/${basename}.lcs\n\"; interact"
  else
    echo "[NOTE] Type \"${YELLOW}execute ${basename}/${basename}.lcs${NC}\" in lc3sim to load the test case."
    echo "[NOTE] To automate this step, either update lc3sim to the latest version or install the tool expect"
    echo "       (For example, on Ubuntu, \"sudo apt-get install expect\")."
    read -p "Press enter to continue to lc3sim"
    lc3sim
  fi
fi
