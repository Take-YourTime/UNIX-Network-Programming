/*
 * lookup1 : straight linear search through a local file
 * 	         of fixed length records. The file name is passed
 *	         as resource.
 */
#include <stdio.h>
#include <string.h>
#include "dict.h"

// Declare in dict.h
// 	sought: sought->word is the word to be looked up in dictionary; 
// 	        sought->text is the text associated with the word, if found.
// 	resource: the dictionary file name 
int lookup(DictRec* sought, const char* resource) {
	DictRec dr;
	static FILE* in;
	static int first_time = 1;

	if (first_time) { 
		first_time = 0;
		/* open up the dictionary file
		 *
		 * Fill in code. */
		in = fopen(resource, "r");
		if (in == NULL) {
			return UNAVAIL;
		}
	}

	/* read from top of dictionary file, looking for match
	 *
	 * Fill in code. */
	rewind(in); // Reset file pointer to the beginning of the file
	
	while(fread(&dr, sizeof(DictRec), 1, in) == 1) {
		/* Fill in code. */
		if (strcmp(dr.word, sought->word) == 0) {
			strcpy(sought->text, dr.text);
			return FOUND;
		}
	}

	return NOTFOUND;
}
