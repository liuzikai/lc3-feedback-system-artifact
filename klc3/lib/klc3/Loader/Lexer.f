/*									tab:8
 *
 * This file is adapted from lc3.f from lc3tools (see tools/lc3tools_release)
 *
 * "Copyright (c) 2003-2021 by Steven S. Lumetta."
 *
 * This file is distributed under the University of Illinois/NCSA Open Source
 * License. See below for details.
 *
 * Author:	    Steve Lumetta and Zikai Liu
 * Version:	    2
 * Creation Date:   18 October 2003
 * Filename:	    lc3.f
 * History:
 *	SSL	1	18 October 2003
 *		Copyright notices and Gnu Public License marker added.
 *	ZKL     2       8 January 2021
 *	    Adapt as part of KLC3Loader.
 */

/* University of Illinois/NCSA Open Source License

Copyright (c) 2003-2021 University of Illinois at Urbana-Champaign.
All rights reserved.

Developed by: Steven S. Lumetta and LIU Tingkai
              University of Illinois at Urbana-Champaign

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation files
(the "Software"), to deal with the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimers.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimers in the
  documentation and/or other materials provided with the distribution.

* Neither the names of University of Illinois at Urbana-Champaign nor the
  names of its contributors may be used to endorse or promote products derived
  from this Software without specific prior written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
THE SOFTWARE. */

%option noyywrap nounput
%option c++
%option yyclass="klc3::KLC3Loader::Lexer"

%{

/* questions...

should the assembler allow colons after label names?  are the colons
part of the label?  Currently I allow only alpha followed by alphanum and _.

*/

#include "klc3/Loader/Loader.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"

%}

/* condition code specification */
CCODE    [Nn]?[Zz]?[Pp]?

/* operand types */
REGISTER [rR][0-7]
HEX      [xX][-]?[0-9a-fA-F]+
DECIMAL  [#]?[-]?[0-9]+
IMMED    {HEX}|{DECIMAL}
LABEL    [A-Za-z][A-Za-z_0-9]*
STRING   \"([^\"]*|(\\\"))*\"
UTSTRING \"[^\n\r]*

/* operand and white space specification */
SPACE     [ \t]
OP_SEP    {SPACE}*,{SPACE}*
COMMENT   [;][^\n\r]*
EMPTYLINE {SPACE}*{COMMENT}?
LINEFEED  \r?\n\r?
ENDLINE   {EMPTYLINE}{LINEFEED}

/* operand formats */
O_RRR  {SPACE}+{REGISTER}{OP_SEP}{REGISTER}{OP_SEP}{REGISTER}{ENDLINE}
O_RRI  {SPACE}+{REGISTER}{OP_SEP}{REGISTER}{OP_SEP}{IMMED}{ENDLINE}
O_RR   {SPACE}+{REGISTER}{OP_SEP}{REGISTER}{ENDLINE}
O_RI   {SPACE}+{REGISTER}{OP_SEP}{IMMED}{ENDLINE}
O_RL   {SPACE}+{REGISTER}{OP_SEP}{LABEL}{ENDLINE}
O_R    {SPACE}+{REGISTER}{ENDLINE}
O_I    {SPACE}+{IMMED}{ENDLINE}
O_L    {SPACE}+{LABEL}{ENDLINE}
O_S    {SPACE}+{STRING}{ENDLINE}
O_UTS  {SPACE}+{UTSTRING}{ENDLINE}
O_     {ENDLINE}

/* need to define YY_INPUT... */

/* exclusive lexing states to read operands, eat garbage lines, and
   check for extra text after .END directive */
%x ls_operands ls_garbage ls_finished

%%

    /* rules for real instruction opcodes */
ADD       {appendLineContent (yytext); inst.op = OP_ADD;   BEGIN (ls_operands);}
AND       {appendLineContent (yytext); inst.op = OP_AND;   BEGIN (ls_operands);}
BR{CCODE} {appendLineContent (yytext); inst.op = OP_BR;    parseCCode (yytext + 2); BEGIN (ls_operands);}
JMP       {appendLineContent (yytext); inst.op = OP_JMP;   BEGIN (ls_operands);}
JSRR      {appendLineContent (yytext); inst.op = OP_JSRR;  BEGIN (ls_operands);}
JSR       {appendLineContent (yytext); inst.op = OP_JSR;   BEGIN (ls_operands);}
LDI       {appendLineContent (yytext); inst.op = OP_LDI;   BEGIN (ls_operands);}
LDR       {appendLineContent (yytext); inst.op = OP_LDR;   BEGIN (ls_operands);}
LD        {appendLineContent (yytext); inst.op = OP_LD;    BEGIN (ls_operands);}
LEA       {appendLineContent (yytext); inst.op = OP_LEA;   BEGIN (ls_operands);}
NOT       {appendLineContent (yytext); inst.op = OP_NOT;   BEGIN (ls_operands);}
RTI       {appendLineContent (yytext); inst.op = OP_RTI;   BEGIN (ls_operands);}
STI       {appendLineContent (yytext); inst.op = OP_STI;   BEGIN (ls_operands);}
STR       {appendLineContent (yytext); inst.op = OP_STR;   BEGIN (ls_operands);}
ST        {appendLineContent (yytext); inst.op = OP_ST;    BEGIN (ls_operands);}
TRAP      {appendLineContent (yytext); inst.op = OP_TRAP;  BEGIN (ls_operands);}

    /* rules for trap pseudo-ols */
GETC      {appendLineContent (yytext); inst.op = OP_GETC;  BEGIN (ls_operands);}
HALT      {appendLineContent (yytext); inst.op = OP_HALT;  BEGIN (ls_operands);}
IN        {appendLineContent (yytext); inst.op = OP_IN;    BEGIN (ls_operands);}
OUT       {appendLineContent (yytext); inst.op = OP_OUT;   BEGIN (ls_operands);}
PUTS      {appendLineContent (yytext); inst.op = OP_PUTS;  BEGIN (ls_operands);}
PUTSP     {appendLineContent (yytext); inst.op = OP_PUTSP; BEGIN (ls_operands);}

    /* rules for non-trap pseudo-ops */
\.FILL    {appendLineContent (yytext); inst.op = OP_FILL;    BEGIN (ls_operands);}
RET       {appendLineContent (yytext); inst.op = OP_RET;     BEGIN (ls_operands);}
\.STRINGZ {appendLineContent (yytext); inst.op = OP_STRINGZ; BEGIN (ls_operands);}

    /* rules for directives */
\.BLKW    {appendLineContent (yytext); inst.op = OP_BLKW; BEGIN (ls_operands);}
\.END     {appendLineContent (yytext); sawEND = 1;        BEGIN (ls_finished);}
\.ORIG    {appendLineContent (yytext); inst.op = OP_ORIG; BEGIN (ls_operands);}

    /* rules for operand formats */
<ls_operands>{O_RRR} {appendLineContent (yytext); generateInstruction (O_RRR, yytext); BEGIN (0);}
<ls_operands>{O_RRI} {appendLineContent (yytext); generateInstruction (O_RRI, yytext); BEGIN (0);}
<ls_operands>{O_RR}  {appendLineContent (yytext); generateInstruction (O_RR, yytext);  BEGIN (0);}
<ls_operands>{O_RI}  {appendLineContent (yytext); generateInstruction (O_RI, yytext);  BEGIN (0);}
<ls_operands>{O_RL}  {appendLineContent (yytext); generateInstruction (O_RL, yytext);  BEGIN (0);}
<ls_operands>{O_R}   {appendLineContent (yytext); generateInstruction (O_R, yytext);   BEGIN (0);}
<ls_operands>{O_I}   {appendLineContent (yytext); generateInstruction (O_I, yytext);   BEGIN (0);}
<ls_operands>{O_L}   {appendLineContent (yytext); generateInstruction (O_L, yytext);   BEGIN (0);}
<ls_operands>{O_S}   {appendLineContent (yytext); generateInstruction (O_S, yytext);   BEGIN (0);}
<ls_operands>{O_}    {appendLineContent (yytext); generateInstruction (O_, yytext);    BEGIN (0);}

    /* eat excess white space */
{SPACE}+ {}
{COMMENT}{LINEFEED}  {appendLineContent (yytext); parseComment (yytext); newInstLine (); /* a line with only comment */}
{LINEFEED} {newInstLine (); /* a blank line */ }

    /* labels, with or without subsequent colons */\
    /*
       the colon form is used in some examples in the second edition
       of the book, but may be removed in the third; it also allows
       labels to use opcode and pseudo-op names, etc., however.
     */
{LABEL}          {/* not include label as line content */ foundLabel (yytext);}
{LABEL}{SPACE}*: {/* not include label as line content */ foundLabel (yytext);}

    /* error handling??? */
<ls_operands>{O_UTS} {unterminatedString (); BEGIN (0);}
<ls_operands>[^\n\r]*{ENDLINE} {badOperands (); BEGIN (0);}
{O_RRR}|{O_RRI}|{O_RR}|{O_RI}|{O_RL}|{O_R}|{O_I}|{O_S}|{O_UTS} {
    badOperands ();
}

. {BEGIN (ls_garbage);}
<ls_garbage>[^\n\r]*{ENDLINE} {badLine (); BEGIN (0);}

    /* parsing after the .END directive */
<ls_finished>{ENDLINE}|{EMPTYLINE}     {newInstLine (); /* a blank line  */}
<ls_finished>.*({ENDLINE}|{EMPTYLINE}) {lineIgnored (); return 0;}

%%

#pragma clang diagnostic pop

void klc3::KLC3Loader::Lexer::resetLexer() {
    BEGIN(0);
}