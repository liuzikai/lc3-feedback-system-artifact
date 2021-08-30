/*									tab:8
 *
 * symbol.h - symbol table interface for the LC-3 assembler and simulator
 *
 * "Copyright (c) 2003-2021 by Steven S. Lumetta."
 *
 * This file is distributed under the University of Illinois/NCSA Open Source
 * License. See LICENSE for details.
 *
 * Author:	    Steve Lumetta
 * Version:	    1
 * Creation Date:   18 October 2003
 * Filename:	    symbol.h
 * History:
 *	SSL	1	18 October 2003
 *		Copyright notices and Gnu Public License marker added.
 *      ZKL/SSL         28 August 2021
 *              Distributed with the University of Illinois/NCSA License
 */

#ifndef SYMBOL_H
#define SYMBOL_H

typedef struct symbol_t symbol_t;
struct symbol_t {
    char* name;
    int addr;
    symbol_t* next_with_hash;
#ifdef MAP_LOCATION_TO_SYMBOL
    symbol_t* next_at_loc;
#endif
};

#define SYMBOL_HASH 997

extern symbol_t* lc3_sym_names[65536];
extern symbol_t* lc3_sym_hash[SYMBOL_HASH];

int symbol_hash (const char* symbol);
int add_symbol (const char* symbol, int addr, int dup_ok);
symbol_t* find_symbol (const char* symbol, int* hptr);
#ifdef MAP_LOCATION_TO_SYMBOL
void remove_symbol_at_addr (int addr);
#endif

#endif /* SYMBOL_H */

