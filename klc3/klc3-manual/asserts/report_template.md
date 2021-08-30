# KLC3 Feedback Tool Report

Report generation time: `{{TIME}}`

Feedback on your commit ([what is this?](https://git-scm.com/book/en/v2/Git-Basics-Recording-Changes-to-the-Repository)): {{COMMIT_ID_AND_LINK}}

## Note

KLC3 is still under test. This report can be **incorrect** or even **misleading**. If you think there is something wrong or unclear, please contact the TAs on [Piazza](link_to_piazza)
(but do not share your code, test cases or reports). Suggestions are also welcomed. Remember that the tool is only to **assist** your work. Even if it can't find any issue, it's **not** guaranteed that you will get the full score, and vice versa.

**If lc3sim on your own machine generates different result than the feedback, first check whether you have used uninitialized memory or registers.**

## Report

{{REPORT}}

## How to Use Test Cases

If an issue is detected, a corresponding test case will be generated in the folder `test*`. The test data is in the asm file. You may copy its content and test your subroutine yourself.

The lcs file is the lc3sim script for you to debug. We have provided a script file for you. Download or checkout this branch. In current folder, run the command:

```
./replay.sh <index>
```

where `<index>` is a decimal index of the test case, and the script will launch lc3sim for you, where you can debug. If you can't execute the script, you may need:

```
chmod +x replay.sh
```

Follow the instruction promoted by the script.

Notice that the replay always uses your code of current commit, rather than your latest code.