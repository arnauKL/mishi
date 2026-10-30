#ifndef CONFIG_H
#define CONFIG_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

/* Així podem activar/desactivar prints de debug fàcilment */
//#define DEBUG

#define PROGRAM_NAME "mishi"
#define MISHI_OK     1
#define MISHI_ERR    -1
#define MISHI_EOF    0

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

#endif // CONFIG_H
