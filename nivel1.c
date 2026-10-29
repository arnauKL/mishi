#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"

int print_prompt();

int
main(void)
{
    char buff[COMMAND_LINE_SIZE];

    while (1) {
        print_prompt();

        // fgets retains newline character.
        if (NULL == fgets(buff, sizeof(buff), stdin)) {
            printf("%s: %s\n", PROGRAM_NAME, strerror(errno));
        }
	}

    return EXIT_SUCCESS;
}

// ---------------------------------

int
print_prompt()
{
    printf("[%s @ %s]$ ", getenv("USER"), getenv("PWD"));
    // faltaria comprovació d errors
    return EXIT_SUCCESS;
}
