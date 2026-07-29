#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

typedef void (*sighandler_t)(int);

#ifndef SIG_HOLD
#define SIG_HOLD ((sighandler_t) 2)
#endif

// set up signal handler
sighandler_t sigset(int sig, sighandler_t handler)
{
    sigset_t set; // create an empty signal set
    if (sigemptyset(&set) == -1)
        return SIG_ERR;

    if (sigaddset(&set, sig) == -1){
    	return SIG_ERR;
    }
	
	// get signal mask to know which signal is blocked
	sigset_t oldmask;
    if (sigprocmask(SIG_SETMASK, NULL, &oldmask) == -1){
    	return SIG_ERR;
    }
        
	
	
	// check if the signal is block or not
	struct sigaction sa, oldsa;
    int was_blocked = sigismember(&oldmask, sig); // the current signal is bloked or not
    if (was_blocked == -1)
        return SIG_ERR;
	
	
	// case 1: block signal
    if (handler == SIG_HOLD) {
        if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
            return SIG_ERR;

        return was_blocked ? SIG_HOLD : SIG_DFL;
    }
	
	
	// case 2: set up handler and unblock signal
    sa.sa_handler = handler; // when meet single, do "handler" program
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // default action (flag is to set up advanced operation of signal handler)
	
	
	// set up signal action to "sa", and save old action in "oldsa"
    if (sigaction(sig, &sa, &oldsa) == -1)
        return SIG_ERR;

    if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1)
        return SIG_ERR;

    return was_blocked ? SIG_HOLD : oldsa.sa_handler;
}

// block signal
// if blocked signal come, it will be pending
int sighold(int sig)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, sig);

    return sigprocmask(SIG_BLOCK, &set, NULL);
}

// unblock signal
int sigrelse(int sig)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, sig);

    return sigprocmask(SIG_UNBLOCK, &set, NULL);
}

// ignore a signal
int sigignore(int sig)
{
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    return sigaction(sig, &sa, NULL);
}


// unblock a signal temporary,
// then wait for the signal
int sigpause(int sig)
{
    sigset_t current_mask;
    sigset_t temp_mask;

    if (sigprocmask(SIG_SETMASK, NULL, &current_mask) == -1)
        return -1;

    temp_mask = current_mask;

    if (sigdelset(&temp_mask, sig) == -1)
        return -1;

    return sigsuspend(&temp_mask);
}



/* ---------- test code ---------- */

static volatile sig_atomic_t got_usr1 = 0;
static volatile sig_atomic_t got_usr2 = 0;

static void handler(int sig)
{
    if (sig == SIGUSR1) {
        got_usr1 = 1;
        write(STDOUT_FILENO, "handler: caught SIGUSR1\n", 24);
    } else if (sig == SIGUSR2) {
        got_usr2 = 1;
        write(STDOUT_FILENO, "handler: caught SIGUSR2\n", 24);
    }
}

static void print_test_result(const char *name, int passed)
{
    printf("[%s] %s\n", passed ? "PASS" : "FAIL", name);
}




int main(void)
{
    sighandler_t old_handler;

    printf("=== System V signal API implementation test ===\n\n");

    /*
     * Test 1: sigset()
     */
    old_handler = sigset(SIGUSR1, handler);
    print_test_result("sigset(SIGUSR1, handler)", old_handler != SIG_ERR);

    raise(SIGUSR1);
    sleep(1);
    print_test_result("SIGUSR1 handler executed", got_usr1 == 1);

    /*
     * Test 2: sighold()
     */
    got_usr1 = 0;

    if (sighold(SIGUSR1) == -1) {
        perror("sighold");
        exit(EXIT_FAILURE);
    }

    raise(SIGUSR1);
    sleep(1);

    print_test_result("sighold(SIGUSR1) blocks signal", got_usr1 == 0);

    /*
     * Test 3: sigrelse()
     *
     * The pending SIGUSR1 should be delivered after unblocking.
     */
    if (sigrelse(SIGUSR1) == -1) {
        perror("sigrelse");
        exit(EXIT_FAILURE);
    }

    sleep(1);
    print_test_result("sigrelse(SIGUSR1) unblocks pending signal", got_usr1 == 1);

    /*
     * Test 4: sigignore()
     */
    got_usr2 = 0;

    if (sigset(SIGUSR2, handler) == SIG_ERR) {
        perror("sigset SIGUSR2");
        exit(EXIT_FAILURE);
    }

    if (sigignore(SIGUSR2) == -1) {
        perror("sigignore");
        exit(EXIT_FAILURE);
    }

    raise(SIGUSR2);
    sleep(1);

    print_test_result("sigignore(SIGUSR2) ignores signal", got_usr2 == 0);

    /*
     * Test 5: sigpause()
     *
     * Block SIGUSR1 first, then fork a child.
     * Child sends SIGUSR1 after 1 second.
     * Parent waits using sigpause(SIGUSR1).
     */
    got_usr1 = 0;

    if (sigset(SIGUSR1, handler) == SIG_ERR) {
        perror("sigset SIGUSR1 again");
        exit(EXIT_FAILURE);
    }

    if (sighold(SIGUSR1) == -1) {
        perror("sighold before sigpause");
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();

    if (child == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        sleep(1);
        kill(getppid(), SIGUSR1);
        _exit(EXIT_SUCCESS);
    }

    printf("calling sigpause(SIGUSR1), waiting for child signal...\n");

    if (sigpause(SIGUSR1) == -1 && errno == EINTR) {
        print_test_result("sigpause(SIGUSR1) waits until signal arrives", got_usr1 == 1);
    } else {
        print_test_result("sigpause(SIGUSR1) waits until signal arrives", 0);
    }

    printf("\n=== Test finished ===\n");

    return 0;
}
