/* utils.h */
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <time.h>

/*
 * Print a message with current timestamp.
 * Example output:
 * [12:34:56] Pushed: char a index 0
 */
static inline void printWithTime(const char *message) {
    time_t now;
    struct tm *timeinfo;
    char timeBuffer[9]; /* HH:MM:SS */

    time(&now);
    timeinfo = localtime(&now);

    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", timeinfo);

    printf("[%s] %s", timeBuffer, message);
    fflush(stdout);
}

#endif /* UTILS_H */

