#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h> // library for getting system information
#include <unistd.h>

int main(void) {
    struct utsname info;

    if(uname(&info) == -1) {｝
        perror("uname");
        exit(EXIT_FAILURE);
    }

    printf("hostname: %s\n", info.nodename);    // print the hostname of the system
    printf("%s\n", info.release);               // print the release version of the operating system
    printf("hostid: %ld\n", gethostid());       // print the host ID of the system as a long integer

    return 0;
}
