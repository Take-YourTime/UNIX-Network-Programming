/*
 * redirect_out.c   :   check for >
 */

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "shell.h"
#include "shell.h"
#define STD_OUTPUT 1
#define STD_INPUT  0

/*
 * Look for ">" in myArgv, then redirect output to the file.
 * Returns 0 on success, sets errno and returns -1 on error.
 */
int redirect_out(int parse_argc, char** myArgv) {
	int i = 0;
  	int fd;

  	/* search forward for >
  	 * Fill in code. */
	while (i < parse_argc && myArgv[i] != NULL && strcmp(myArgv[i], ">") != 0) {
        i++;
    }
    
  	if (myArgv[i]) {	/* found ">" in vector. */

    	/* 1) Open file.
    	 * 2) Redirect to use it for output.
    	 * 3) Cleanup / close unneeded file descriptors.
    	 * 4) Remove the ">" and the filename from myArgv.
		 *
    	 * Fill in code. */

        // lose target output file
    	if (myArgv[i + 1] == NULL || i >= parse_argc - 1) {
            errno = EINVAL;
            return -1;
        }

        fd = open(myArgv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            return -1;
        }

        // redirect fd to stdout to use file for output
        if (dup2(fd, STD_OUTPUT) < 0) {
            close(fd);
            return -1;
        }

        close(fd);
        myArgv[i] = NULL;
        myArgv[i+1] = NULL;
  	}
  	return 0;
}
