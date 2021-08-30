/*									tab:8
 *
 * symbol.c - symbol table functions for the LC-3 assembler and simulator
 *
 * "Copyright (c) 2003-2021 by Steven S. Lumetta."
 *
 * This file is distributed under the University of Illinois/NCSA Open Source
 * License. See LICENSE for details.
 *
 * Author:	    Steve Lumetta
 * Version:	    1
 * Creation Date:   18 October 2003
 * Filename:	    symbol.c
 * History:
 *	SSL	1	18 October 2003
 *		Copyright notices and Gnu Public License marker added.
 *	SSL	2	9 October 2020
 *		Eliminated warning for empty loop body.
 *      ZKL/SSL         28 August 2021
 *              Distributed with the University of Illinois/NCSA License
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "symbol.h"

symbol_t* lc3_sym_hash[SYMBOL_HASH];
#ifdef MAP_LOCATION_TO_SYMBOL
symbol_t* lc3_sym_names[65536];
#endif

int
symbol_hash (const char* symbol)
{
    int h = 1;

    while (*symbol != 0)
	h = (h * tolower (*symbol++)) % SYMBOL_HASH;

    return h;
}

symbol_t*
find_symbol (const char* symbol, int* hptr)
{
    int h = symbol_hash (symbol);
    symbol_t* sym;

    if (hptr != NULL)
        *hptr = h;
    for (sym = lc3_sym_hash[h]; sym != NULL; sym = sym->next_with_hash)
    	if (strcasecmp (symbol, sym->name) == 0)
	    return sym;
    return NULL;
}

int
add_symbol (const char* symbol, int addr, int dup_ok)
{
    int h;
    symbol_t* sym;

    if ((sym = find_symbol (symbol, &h)) == NULL) {
	sym = (symbol_t*)malloc (sizeof (symbol_t)); 
	sym->name = strdup (symbol);
	sym->next_with_hash = lc3_sym_hash[h];
	lc3_sym_hash[h] = sym;
#ifdef MAP_LOCATION_TO_SYMBOL
	sym->next_at_loc = lc3_sym_names[addr];
	lc3_sym_names[addr] = sym;
#endif
    } else if (!dup_ok) 
        return -1;
    sym->addr = addr;
    return 0;
}


#ifdef MAP_LOCATION_TO_SYMBOL
void
remove_symbol_at_addr (int addr)
{
    symbol_t* s;
    symbol_t** find;
    int h;

    while ((s = lc3_sym_names[addr]) != NULL) {
        h = symbol_hash (s->name);
	for (find = &lc3_sym_hash[h]; *find != s; 
	     find = &(*find)->next_with_hash) { }
        *find = s->next_with_hash;
	lc3_sym_names[addr] = s->next_at_loc;
	free (s->name);
	free (s);
    }
}
#endif

