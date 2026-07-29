// print the current date and time in the format "Mon 01 Jan 2000 12:00:00 PM"

#include <stdio.h>
#include <time.h>

int main(void) {
    time_t now;
    struct tm* tm_info;
    char output_buffer[100];

    time(&now); // get current time
                // the number means the seconds since the epoch (January 1, 1970)
                // p.s. 也可以寫成 time_t now = time(NULL);
    
    tm_info = localtime(&now); // turn time_t into struct tm, which is easier to read and format
    if (tm_info == NULL) {
        perror("localtime");
        return 1;
    }


    // Turn struct tm into a string with the desired format.
    //      "%b %d(%a), %Y %I:%M %p" means "Jan 01(Sun), 2000 12:00 PM"
    //      see ReadMe to learn more about strftime format specifiers
    if (strftime(output_buffer, sizeof(output_buffer), "%b %d(%a), %Y %I:%M %p", tm_info) == 0) {
        fprintf(stderr, "strftime buffer too small\n");
        return 1;
    }

    printf("%s\n", output_buffer);
    return 0;
}
