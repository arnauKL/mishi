#ifndef CONFIG_H
#define CONFIG_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <unistd.h>

/* Així podem activar/desactivar prints de debug fàcilment */
#define DEBUG

// https://stackoverflow.com/a/1941336
#ifdef DEBUG
#define DEBUG_PRINT(...) fprintf( stderr, __VA_ARGS__ )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif


#define PROGRAM_NAME "mishi"
#define MISHI_OK     0
#define MISHI_ERR    -1
#define MISHI_EOF    2

#define MAX_PATH_LEN      128
#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE         64
#define TOKEN_DELIMITER   " \t\r\n" // x ara: espai, tab, newline

/* declar nou tipus `builtin_fn` tipus punter a funció:
 * retorna `int` i reb `char **`
 */
typedef int (*builtin_fn)(char **);

/* struct per guardar builtins amb el nom "cd" i la funció interna
 * "mishi_cd" */
struct builtin
{
    char *name;
    builtin_fn func;
};

/* struct per guardar informació del job
 * char estat no estaria millor fer q sigui un ENUM?
 */
struct info_job
{
	pid_t pid;
	char estat;  // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: Ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado)  

    char cmd[COMMAND_LINE_SIZE]; // línea de comand associada
};

#endif // CONFIG_H
