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

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <config.h>

typedef struct HashEntry {
    union {
	char *string;		       /* the entry key. can be a 
					* int value a pointer or a string 
					* value */
	int number;
	void *ptr;
    } key;
    void *data;			       /* the data stored in the entry */
    struct HashEntry *next;	       /* next on the linked-list for 
					* collisions */
} HashEntry;


/*
 * A generic hash table structure
 */
typedef struct HashTable {
    int elements;		       /* elements stored in the table */
    int size;			       /* size of the table */
    HashEntry **table;
} HashTable;



HashTable *table_init(HashTable *table);
void table_sdestroy(HashTable *table);
void table_sput(HashTable *table, char *key, void *data);
void *table_sget(HashTable *table, char *key);
void table_sdelete(HashTable *table, char *key);

void table_idestroy(HashTable *table);
void table_iput(HashTable *table, int key, void *data);
void *table_iget(HashTable *table, int key);
void table_idelete(HashTable *table, int key);

void table_pdestroy(HashTable *table);
void table_pput(HashTable *table, void *key, void *data);
void *table_pget(HashTable *table, void *key);
void table_pdelete(HashTable *table, void *key);

void *table_index(HashTable *table, int index);


#endif
