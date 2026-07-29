/*
 * convert.c : take a file in the form 
 *  word1
 *  multiline definition of word1
 *  stretching over several lines, 
 * followed by a blank line
 * word2....etc
 * convert into a file of fixed-length records
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dict.h"
#define BIGLINE 512

int main(int argc, char **argv) {
	FILE *in;
	FILE *out;        /* defaults */
	char line[BIGLINE];
	static Dictrec dr, blank;
	
	/* If args are supplied, argv[1] is for input, argv[2] for output */
	if (argc==3) {
		if ((in =fopen(argv[1],"r")) == NULL){DIE(argv[1]);}
		if ((out =fopen(argv[2],"w")) == NULL){DIE(argv[2]);}	
	}
	else{
		printf("Usage: convert [input file] [output file].\n");
		return -1;
	}

	/* Main reading loop : read word first, then definition into dr */

	/* Loop through the whole file. */
	while (fgets(line, sizeof(line), in) != NULL) {
		
		/* Create and fill in a new blank record.
		 * First get a word and put it in the word field, then get the definition
		 * and put it in the text field at the right offset.  Pad the unused chars
		 * in both fields with nulls.
		 */

		/* Read word and put in record.  Truncate at the end of the "word" field.
		 *
		 * Fill in code. */
		int offset = 0;
		size_t len;

		dr = blank;
		memset(&dr, 0, sizeof(Dictrec));

		len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
		}

		strncpy(dr.word, line, WORD - 1);
		dr.word[WORD - 1] = '\0';
		
		/* Read definition, line by line, and put in record.
		 *
		 * Fill in code. */
		while (fgets(line, sizeof(line), in) != NULL) {
			if (strcmp(line, "\n") == 0) {
				break;
			}

			len = strlen(line);
			if (offset + (int)len >= TEXT) {
				len = TEXT - offset - 1;
			}

			if (len > 0) {
				memcpy(dr.text + offset, line, len);
				offset += (int)len;
			}

			if (offset >= TEXT - 1) {
				dr.text[TEXT - 1] = '\0';
				break;
			}
		}
		
		/* Write record out to file.
		 *
		 * Fill in code. */
		dr.text[TEXT - 1] = '\0';

		if (fwrite(&dr, sizeof(Dictrec), 1, out) != 1) {
			DIE("fwrite");
		}
	}

	fclose(in);
	fclose(out);
	return 0;
}
