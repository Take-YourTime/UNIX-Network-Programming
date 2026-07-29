/*
 * is_background.c :  check for & at end
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"

int is_background(char** myArgv) {

  	if (*myArgv == NULL)
    	return 0;

  	/* Look for "&" in myArgv, and process it.
  	 *
	 *	- Return TRUE if found.
	 *	- Return FALSE if not found.
	 *
	 * Fill in code.
	 */
	if (myArgv[0] == NULL) return 0;

	int i = 0; // argument index

	// find the last element in myArgv
	while(myArgv[i+1] != NULL){
		i++;
	}
	
	if(myArgv[i][0] == '&' && strlen(myArgv[i]) == 1){
		free(myArgv[i]);
		myArgv[i] = NULL;
		return 1;
	}
	else return 0;
}
