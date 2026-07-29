#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int numSigs;
    int sig;
    pid_t childPid;
    sigset_t blockMask;
    siginfo_t si;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s num-signals\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    numSigs = atoi(argv[1]);
    if (numSigs <= 0) {
        fprintf(stderr, "num-signals must be > 0\n");
        exit(EXIT_FAILURE);
    }

    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGUSR1);

    /*
     * Important:
     * SIGUSR1 must be blocked before fork().
     * Then both parent and child inherit the blocked mask.
     * sigwaitinfo() can then synchronously receive SIGUSR1.
     */
    if (sigprocmask(SIG_BLOCK, &blockMask, NULL) == -1)
        errExit("sigprocmask");

    childPid = fork();

    if (childPid == -1)
        errExit("fork");

    if (childPid == 0) {
        /*
         * Child:
         * Wait for SIGUSR1 from parent.
         * Then send SIGUSR1 back to parent.
         */
        for (;;) {
            sig = sigwaitinfo(&blockMask, &si);
            if (sig == -1)
                errExit("sigwaitinfo child");

            if (kill(getppid(), SIGUSR1) == -1)
                errExit("kill child");
        }

        _exit(EXIT_SUCCESS);
    }

    /*
     * Parent:
     * Send one signal to child.
     * Wait for reply.
     * Repeat numSigs times.
     */
    for (int i = 0; i < numSigs; i++) {
        if (kill(childPid, SIGUSR1) == -1)
            errExit("kill parent");

        sig = sigwaitinfo(&blockMask, &si);
        if (sig == -1)
            errExit("sigwaitinfo parent");
    }

    kill(childPid, SIGTERM);
    wait(NULL);

    exit(EXIT_SUCCESS);
}