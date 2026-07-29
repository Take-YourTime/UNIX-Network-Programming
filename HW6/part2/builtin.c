/*
 * builtin.c : check for shell built-in commands
 * structure of file is
 * 1. definition of builtin functions
 * 2. lookup-table
 * 3. definition of is_builtin and do_builtin
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/types.h>
#include "shell.h"

#define PATH_MAX 100
/****************************************************************************/
/* builtin function definitions                                             */
/****************************************************************************/
static void bi_builtin(char** argv);	/* "builtin" command tells whether a command is builtin or not. */
static void bi_echo(char** argv);	/* "echo" command.  Does not print final <CR> if "-n" encountered. */
static void bi_cd(char** argv) ;		/* "cd" command. */
static void bi_hostname(char** argv);	/* "hostname" command. */
static void bi_id(char** argv);		/* "id" command shows user and group of this process. */
static void bi_pwd(char** argv);		/* "pwd" command. */
static void bi_quit(char** argv);		/* quit/exit/logout/bye command. */




/****************************************************************************/
/* lookup table                                                             */
/****************************************************************************/

static struct cmd {
  	char * keyword;					/* When this field is argv[0] ... */
  	void (* do_it)(char **);		/* ... this function is executed. */
} inbuilts[] = {
  	{ "builtin",    bi_builtin },   /* List of (argv[0], function) pairs. */

    /* Fill in code. */
    { "echo",       bi_echo },
    { "quit",       bi_quit },
    { "exit",       bi_quit },
    { "bye",        bi_quit },
    { "logout",     bi_quit },
    { "cd",         bi_cd },
    { "pwd",        bi_pwd },
    { "id",         bi_id },
    { "hostname",   bi_hostname },
    {  NULL,        NULL }          /* NULL terminated. */
};


static void bi_builtin(char** argv) {
    if (argv[1] == NULL) {
        // output all the built-in commands
        int i;
        for (i = 0; inbuilts[i].keyword != NULL; i++) {
            printf("%s\n", inbuilts[i].keyword);
        }
        return;
    }

    // Check if argv[1] is a built-in command
    if (is_builtin(argv[1])) {
        printf("%s is a builtin feature.\n", argv[1]);
    } else {
        printf("%s is NOT a builtin feature.\n", argv[1]);
    }
}

/* "echo" command.  Does not print final <CR> if "-n" encountered. */
static void bi_echo(char** argv) {
  	/* Fill in code. */
  	
	int i; // the index of argv[]
	int newLine; // Flag: print \n or not
	
	// option -n
	if(argv[1] != NULL && strcmp(argv[1], "-n") == 0){
		newLine = 0;
		i = 2;
	}
	else{
		newLine = 1;
		i = 1;
	}
	
	while(argv[i] != NULL){
		printf("%s", argv[i]);
		if(argv[i+1] != NULL) printf(" ");
		
		i++;
	}
	
	if(newLine) printf("\n");
}

static void bi_cd(char** argv) {
    char *path = argv[1];

    if (path == NULL) {
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    }

    if (chdir(path) != 0) {
        perror("cd");
    }
}

static void bi_hostname(char** argv) {
    struct utsname u;

    if (uname(&u) == -1) {
        perror("hostname");
        return;
    }

    printf("hostname: %s\n", u.nodename);
}

static void bi_id(char** argv) {
    uid_t uid;
    gid_t gid;
    struct passwd *pw;
    struct group *gr;

    (void)argv;

    uid = getuid();
    gid = getgid();

    pw = getpwuid(uid);
    gr = getgrgid(gid);

    printf("UserID = %d", (int)uid);
    if (pw != NULL) {
        printf("(%s)", pw->pw_name);
    }

    printf(", GroupID = %d", (int)gid);
    if (gr != NULL) {
        printf("(%s)", gr->gr_name);
    }

    printf("\n");
}

static void bi_pwd(char** argv) {
    char cwd[PATH_MAX];

    (void)argv;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return;
    }

    printf("%s\n", cwd);
}

static void bi_quit(char **argv) {
    (void)argv;
    exit(0);
}


/****************************************************************************/
/* is_builtin and do_builtin                                                */
/****************************************************************************/

static struct cmd * this; /* close coupling between is_builtin & do_builtin */

/* Check to see if command is in the inbuilts table above.
Hold handle to it if it is. */
int is_builtin(char *cmd) {
	struct cmd *tableCommand;

  	for (tableCommand = inbuilts ; tableCommand->keyword != NULL; tableCommand++)
    	if (strcmp(tableCommand->keyword,cmd) == 0) {
      		this = tableCommand;
      		return 1;
    	}
  return 0;
}


/* Execute the function corresponding to the builtin cmd found by is_builtin. */
int do_builtin(char **argv) {
	if(argv == NULL && argv[0] == NULL)
		return EXIT_FAILURE;
	
	this->do_it(argv);
	
	return EXIT_SUCCESS;
}
