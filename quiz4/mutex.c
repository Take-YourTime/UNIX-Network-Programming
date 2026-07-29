#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "stack.h"
#include "utils.h"

#define STACK_SIZE 3

static char buffer[STACK_SIZE];
static int index = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void push(char oneChar)
{
    char string[64];

    pthread_mutex_lock(&mutex);
	
	// stack full
    while (index == STACK_SIZE) {
        printWithTime("push failed: stack full\n");
        pthread_mutex_unlock(&mutex);
        //return -1;
    }

    buffer[index++] = oneChar;

    sprintf(string, "Pushed:\tchar %c\tindex %d\n", oneChar, index - 1);
    printWithTime(string);

    pthread_mutex_unlock(&mutex);
}

char pop()
{
    char string[64];

    pthread_mutex_lock(&mutex);

    if (index == 0) {
        printWithTime("pop failed: stack empty\n");
        pthread_mutex_unlock(&mutex);
        return '\0';
    }

    char toReturn = buffer[--index];

    sprintf(string, "Pop:\tchar %c\tindex %d\n", toReturn, index);
    printWithTime(string);

    pthread_mutex_unlock(&mutex);

    return toReturn;
}

void *push_thread(void *arg)
{
    char ch = *(char *)arg;
    push(ch); // use busy-waiting
    return NULL;
}

int main(void)
{
    pthread_t tid;
    char d = 'D';

    push('A');
    push('B');
    push('C');

    pthread_create(&tid, NULL, push_thread, &d);
	
	
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 100;
	nanosleep(&ts, NULL);   // sleep 100 nano-second

    pop();

    pthread_join(tid, NULL);

    return 0;
}
