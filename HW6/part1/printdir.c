// print the current working directory to standard output

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    char* cwd = getcwd(NULL, 0); // get current working directory, dynamically allocated

    if (cwd == NULL) {
        perror("getcwd");
        return 1;
    }

    printf("%s\n", cwd);
    free(cwd);

    return 0;
}
