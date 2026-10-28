#include <stdio.h>
#include <stdlib.h>

// #include <readline.h> // 

#include "constants.h"

int
main(void)
{
    printf("%s v.%s\n", PROGRAM_NAME, PROGRAM_VERS);



    while(1)
    {
        printf("%s", PROMPT);
    };

    return EXIT_SUCCESS;
}
