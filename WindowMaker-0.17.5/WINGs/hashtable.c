/*
 *  WindowMaker miscelaneous function library
 * 
 *  Copyright (c) 1997 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "../src/config.h"

#include "WUtil.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "hashtable.h"

#define INIT_TABLE_SIZE	31

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define INLINE inline
#else
# define INLINE
#endif


/*
 *----------------------------------------------------------------------
 * table_init--
 * 	Initializes a hashtable with a initial size = INIT_TABLE_SIZE
 * The table must already be allocated.
 * 
 * Returns:
 * 	The table.
 *---------------------------------------------------------------------- 
 */
HashTable *
table_init(HashTable *table)
{
    table->elements = 0;
    table->size = INIT_TABLE_SIZE;
    table->table = wmalloc(INIT_TABLE_SIZE*sizeof(HashEntry*));
    memset(table->table, 0, INIT_TABLE_SIZE*sizeof(HashEntry*));

    return table;
}


/*
 *----------------------------------------------------------------------
 * table_sdestroy--
 * 	Frees a string keyed hashtable and all entries in it.
 * 
 *---------------------------------------------------------------------- 
 */
void
table_sdestroy(HashTable *table)
{
    HashEntry *entry, *next;
    int i;

    for (i=0; i<table->size; i++) {
	entry=table->table[i];
	while (entry) {
	    next = entry->next;
	    free(entry);
	    entry = next;
	}
    }
    free(table->table);
}


static INLINE unsigned long
hash_skey(char *key)
{
    unsigned long h = 0, g;
    
    while(*key) {
	h = (h<<4) + *key++;
	if ((g = h & 0xf0000000))
	  h ^=g >> 24;
	h &= ~g;
    }
    return h;
}


static INLINE unsigned long
hash_ikey(int key)
{
    unsigned long h = 0, g;
    int i;
    char skey[sizeof(int)];

    memcpy(skey, &key, sizeof(int));
    for (i=0; i<sizeof(int); i++) {
	h = (h<<4) + skey[i];
	if ((g = h & 0xf0000000))
	  h ^=g >> 24;
	h &= ~g;
    }
    return h;
}


static INLINE unsigned long
hash_pkey(void *key)
{
    return ((unsigned long)key / sizeof(void *));
}


static void
rebuild_stable(HashTable *table)
{
    HashTable newtable;
    HashEntry *entry;
    int i;

    newtable.elements = 0;
    newtable.size = table->size*2;
    newtable.table = wmalloc(newtable.size * sizeof(HashEntry*));
    memset(newtable.table, 0, newtable.size * sizeof(HashEntry*));    
    for (i=0; i<table->size; i++) {
	entry = table->table[i];
	while (entry) {
	    table_sput(&newtable, entry->key.string, entry->data);
	    entry=entry->next;
	}
    }
    table_sdestroy(table);
    table->elements = newtable.elements;
    table->size = newtable.size;
    table->table = newtable.table;
}

/*
 *---------------------------------------------------------------------- 
 * table_sput--
 * 	Put an entry in a string key hash table. If the collisions
 * are too much, the table is rebuilt.
 * 
 * Side effects:
 * 	A new entry is allocated and inserted on the table. 
 * The table may be completely rebuilt. The key is duplicated in a 
 * newly allocated area.
 *---------------------------------------------------------------------- 
 */
void
table_sput(HashTable *table, char *key, void *data)
{
    unsigned long hkey;
    HashEntry *nentry;
    
    nentry = wmalloc(sizeof(HashEntry));
    nentry->key.string = wstrdup(key);
    nentry->data = data;
    hkey = hash_skey(key) % table->size;
    /* collided */
    if (table->table[hkey]!=NULL) {
	nentry->next = table->table[hkey];
	table->table[hkey] = nentry;
    } else {
	nentry->next = NULL;
	table->table[hkey] = nentry;
    }
    table->elements++;

    if (table->elements>(table->size*3)/2) {
#ifdef DEBUG
	printf("rebuilding string hash table...\n");
#endif
	rebuild_stable(table);
#ifdef DEBUG
	printf("end rebuild\n");
#endif
    }
}


/*
 *---------------------------------------------------------------------- 
 * table_sget--
 * 	Get an entry in a string key hash table.
 * 
 * Returns:
 * 	NULL if the entry is not on the list or the data in the entry.
 * 
 * Side effects:
 * 	None
 *---------------------------------------------------------------------- 
 */
void *
table_sget(HashTable *table, char *key)
{
    unsigned long hkey;
    HashEntry *entry;
    
    hkey = hash_skey(key)%table->size;
    entry = table->table[hkey];
    while (entry!=NULL) {
	if (strcmp(entry->key.string, key)==0) {
	    return entry->data;
	}
	entry = entry->next;
    }
    return NULL;
}



static HashEntry *
delete_fromslist(HashTable *table, HashEntry *entry, char *key)
{
    HashEntry *next;

    if (entry==NULL) return NULL;
    if (strcmp(entry->key.string, key)==0) {
	next = entry->next;
	free(entry->key.string);
	free(entry);
	return next;
    }
    entry->next = delete_fromslist(table, entry->next, key);
    return entry;
}

/*
 *---------------------------------------------------------------------- 
 * table_sdelete--
 * 	Deletes an entry from a hashtable
 * 
 *---------------------------------------------------------------------- 
 */
void
table_sdelete(HashTable *table, char *key)
{
    unsigned long hkey;
    
    hkey = hash_skey(key)%table->size;
    table->table[hkey] = delete_fromslist(table, table->table[hkey], key);
    table->elements--;
}


static void
rebuild_itable(HashTable *table)
{
    HashTable newtable;
    HashEntry *entry;
    int i;

    newtable.elements = 0;
    newtable.size = table->size*2;
    newtable.table = wmalloc(newtable.size * sizeof(HashEntry*));
    memset(newtable.table, 0, newtable.size * sizeof(HashEntry*));
    for (i=0; i<table->size; i++) {
	entry = table->table[i];
	while (entry) {
	    table_iput(&newtable, entry->key.number, entry->data);
	    entry=entry->next;
	}
    }
    table_idestroy(table);
    table->elements = newtable.elements;
    table->size = newtable.size;
    table->table = newtable.table;
}


void
table_iput(HashTable *table, int key, void *data)
{
    unsigned long hkey;
    HashEntry *nentry;
    
    nentry = wmalloc(sizeof(HashEntry));
    nentry->key.number = key;
    nentry->data = data;
    hkey = hash_ikey(key) % table->size;
    /* collided */
    if (table->table[hkey]!=NULL) {
	nentry->next = table->table[hkey];
	table->table[hkey] = nentry;
    } else {
	nentry->next = NULL;
	table->table[hkey] = nentry;
    }
    table->elements++;
        
    if (table->elements>(table->size*3)/2) {
#ifdef DEBUG
	printf("rebuilding integer hash table...\n");
#endif
	rebuild_itable(table);
#ifdef DEBUG
	printf("end rebuild");
#endif
    }
}


void *
table_iget(HashTable *table, int key)
{
    unsigned long hkey;
    HashEntry *entry;
    
    hkey = hash_ikey(key)%table->size;
    entry = table->table[hkey];
    while (entry!=NULL) {
	if (entry->key.number==key) {
	    return entry->data;
	}
	entry = entry->next;
    }
    return NULL;
}



static HashEntry *
delete_fromilist(HashTable *table, HashEntry *entry, int key)
{
    HashEntry *next;

    if (entry==NULL) return NULL;
    if (entry->key.number==key) {
	next = entry->next;
	free(entry);
	return next;
    }
    entry->next = delete_fromilist(table, entry->next, key);
    return entry;
}

void
table_idelete(HashTable *table, int key)
{
    unsigned long hkey;
    
    hkey = hash_ikey(key)%table->size;
    table->table[hkey] = delete_fromilist(table, table->table[hkey], key);
    table->elements--;
}


void
table_idestroy(HashTable *table)
{
    HashEntry *entry, *next;
    int i;

    for (i=0; i<table->size; i++) {
	entry=table->table[i];
	while (entry) {
	    next = entry->next;
	    entry = next;
	}
    }
    free(table->table);
}


void *
table_index(HashTable *table, int index)
{
    HashEntry *entry=NULL;
    int i;

    if (index<0 || index>table->elements) return NULL;
    
    for (i=0; i<table->size; i++) {
	entry=table->table[i];
	if (entry) {
	    while (index>0 && entry) {
		index--;
		entry=entry->next;
	    }
	}
	if (index==0)
	  break;
    }
    if (!entry)
      return NULL;
    else
      return entry->data;
}


static void
rebuild_ptable(HashTable *table)
{
    HashTable newtable;
    HashEntry *entry;
    int i;

    newtable.elements = 0;
    newtable.size = table->size*2;
    newtable.table = wmalloc(newtable.size * sizeof(HashEntry*));
    memset(newtable.table, 0, newtable.size * sizeof(HashEntry*));
    for (i=0; i<table->size; i++) {
	entry = table->table[i];
	while (entry) {
	    table_pput(&newtable, entry->key.ptr, entry->data);
	    entry=entry->next;
	}
    }
    table_pdestroy(table);
    table->elements = newtable.elements;
    table->size = newtable.size;
    table->table = newtable.table;
}


void
table_pput(HashTable *table, void *key, void *data)
{
    unsigned long hkey;
    HashEntry *nentry;
    
    nentry = wmalloc(sizeof(HashEntry));
    nentry->key.ptr = key;
    nentry->data = data;
    hkey = hash_pkey(key) % table->size;
    /* collided */
    if (table->table[hkey]!=NULL) {
	nentry->next = table->table[hkey];
	table->table[hkey] = nentry;
    } else {
	nentry->next = NULL;
	table->table[hkey] = nentry;
    }
    table->elements++;
        
    if (table->elements>(table->size*3)/2) {
#ifdef DEBUG
	printf("rebuilding ptr hash table...\n");
#endif
	rebuild_ptable(table);
#ifdef DEBUG
	printf("end rebuild\n");
#endif
    }
}


void *
table_pget(HashTable *table, void *key)
{
    unsigned long hkey;
    HashEntry *entry;
    
    hkey = hash_pkey(key)%table->size;
    entry = table->table[hkey];
    while (entry!=NULL) {
	if (entry->key.ptr==key) {
	    return entry->data;
	}
	entry = entry->next;
    }
    return NULL;
}



static HashEntry *
delete_fromplist(HashTable *table, HashEntry *entry, void *key)
{
    HashEntry *next;

    if (entry==NULL) return NULL;
    if (entry->key.ptr==key) {
	next = entry->next;
	free(entry);
	return next;
    }
    entry->next = delete_fromplist(table, entry->next, key);
    return entry;
}

void
table_pdelete(HashTable *table, void *key)
{
    unsigned long hkey;
    
    hkey = hash_pkey(key)%table->size;
    table->table[hkey] = delete_fromplist(table, table->table[hkey], key);
    table->elements--;
}


void
table_pdestroy(HashTable *table)
{
    HashEntry *entry, *next;
    int i;

    for (i=0; i<table->size; i++) {
	entry=table->table[i];
	while (entry) {
	    next = entry->next;
	    entry = next;
	}
    }
    free(table->table);
}
