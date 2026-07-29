#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static volatile sig_atomic_t gotSig = 0;

static void handler(int sig)
{
    (void)sig;
    gotSig = 1;
}

static void errExit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int numSigs;
    pid_t childPid;
    sigset_t blockMask, emptyMask;
    struct sigaction sa;

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
    sigemptyset(&emptyMask);

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        errExit("sigaction");

    if (sigprocmask(SIG_BLOCK, &blockMask, NULL) == -1)
        errExit("sigprocmask");

    childPid = fork();
    if (childPid == -1)
        errExit("fork");

    if (childPid == 0) {
        for (;;) {
            while (!gotSig)
                sigsuspend(&emptyMask);

            gotSig = 0;

            if (kill(getppid(), SIGUSR1) == -1)
                errExit("child kill");
        }
    }

    for (int i = 0; i < numSigs; i++) {
        gotSig = 0;

        if (kill(childPid, SIGUSR1) == -1)
            errExit("parent kill");

        while (!gotSig)
            sigsuspend(&emptyMask);
    }

    kill(childPid, SIGTERM);
    wait(NULL);

    return 0;
}
