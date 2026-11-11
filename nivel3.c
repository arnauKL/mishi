/*
 * Novembre 2026
 *
 * Minishell Sistemes Operatius, nivell 3.
 *
 * Grup 101, subgrup "cé menos menos":
 * 		- Jaume Cladera
 * 		- Arnau K. Deprez
 * 		- Matías Volpe
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


/* =-------------------= configuracions =-------------------= */

#define PROGRAM_NAME "mishi"
#define MISHI_OK     0
#define MISHI_ERR    -1
#define MISHI_EOF    2

#define MAX_PATH_LEN      128
#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE         64
#define TOKEN_DELIMITER   " \t\r\n" // x ara: espai, tab, newline

#define MAX_JOBS 10

/* Així podem activar/desactivar prints de debug fàcilment */
#define DEBUG

/* =-------------------= macros =-------------------= */

/* https://stackoverflow.com/a/1941336 */
#ifdef DEBUG
#define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...) do { } while (0)
#endif

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

/* =-------------------= nous tipus =-------------------= */

/*
 * tipus `builtin_fn` tipus punter a funció:
 * retorna `int` i reb `char **`
 */
typedef int (*builtin_fn)(char **);

/*
 * struct per guardar builtins amb el nom p. ex. "cd" i la funció
 * interna p. ex "internal_cd"
 */
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

// TODO: veure si això pot ser útil
static const char[] = {'N', 'E', 'D', 'F'};

/* =-------------------= prototips =-------------------= */
int execute(char **args);
int execute_line(char *line);
int launch(char **args);
int internal_loop();
int print_prompt();
char *read_line(char *line);
int parse_args(char *line, char **args);
void strip_newline(char *line);

/* Builtins */
int internal_bg(char **args);
int internal_cd(char **args);
int internal_exit(char **args);
int internal_export(char **args);
int internal_fg(char **args);
int internal_help(char **args); // no feia falta ficar-la però era fàcil d afegir
int internal_jobs(char **args);
int internal_source(char **args);

/* =-------------------= globals =-------------------= */

/* array que guarda tots els builtins disponibles (nom i punter a la funció) */
struct builtin builtins[] = {
    // clang-format off
    { "cd"     , internal_cd},
    { "exit"   , internal_exit},
    { "export" , internal_export },
    { "fg"     , internal_fg },
    { "help"   , internal_help},
    { "jobs"   , internal_jobs },
    { "source" , internal_source },
    { "." 	   , internal_source }, // alias com a bash
    // clang-format on
};

/* =-------------------= implementacions =-------------------= */
/* només crida a loop */
int main(void) {
    return internal_loop();
}

/*
 * executa el primer element de l'array de comands `args`
 * comprova si és un builtin i l'executa si ho és.
 *
 * retorna el valor de retorn de la funció o 1 en cas que args sigui NULL o
 * estigui  buit
 */
int execute(char **args) {
    /* mirar usuari ha passat un command buit */
    if (args == NULL || args[0] == NULL) {
        return MISHI_OK;
    }

    /* primer mirar si és algun dels nostres builtins */
    for (size_t i = 0; i < ARRAY_LEN(builtins); i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            DEBUG_PRINT("[%s()]\n", builtins[i].name);
            return builtins[i].func(args);
        }
    }

    return launch(args);
}

/*
 * S'hi passa una línia sencera acabada amb \0.
 * line es destrueix i es divideix en una llista de strings (args)
 * S'executa la llista de strings (args)
 * retorna el valor de retorn de la comanda executada.
 */
int execute_line(char *line) {

    char *args[ARGS_SIZE] = {0};
    int n_args = parse_args(line, args);
    if (n_args == 0) {
        return MISHI_OK;
    }

#ifdef DEBUG /* mostrar args */
    fprintf(stderr, "[parse_args() -> ");
    for (int i = 0; args[i] != NULL; i++) {
        fprintf(stderr, " '%s'", args[i]);
    }
    fprintf(stderr, "]\n");
#endif // DEBUG

    /* en teoria només s'ha de passar a check_internal() però ja directament
     * miram si és un internal dins execute i executam allà */

    return execute(args);
}

/* reb llista de strings amb els arguments de cd i canvia el directori */
int internal_cd(char **args)
{
    const char *target;

    if (args[1] == NULL) {
        /* cas que ens passen `cd` tot sol -> anar a $HOME */
        target = getenv("HOME");
        if (target == NULL) {
            fprintf(stderr, "%s: cd: HOME is not set\n", PROGRAM_NAME);
            return MISHI_ERR;
        }
    } else if (args[2] != NULL) {
        /* cas que ens passen `cd` amb massa arguments */
        fprintf(stderr, "%s: cd: too many arguments\n", PROGRAM_NAME);
        return MISHI_ERR;
    } else if (strncmp(args[1], "-", 1) == 0) {
        /* cas que ens passen `cd -` -> anar a $OLDPWD */
        target = getenv("OLDPWD");
        if (target == NULL) {
            fprintf(stderr, "%s: cd: OLDPWD is not set\n", PROGRAM_NAME);
            return MISHI_ERR;
        }
    } else {
        /* tots els altres casos: `cd ex_dir` */
        target = args[1];
    }

    if (chdir(target) != 0) {
        fprintf(stderr, "%s: cd: %s: %s\n",
                PROGRAM_NAME, target, strerror(errno));
        return MISHI_ERR;
    }

    /* Actualitzar PWD i OLDPWD. */
    char buf[MAX_PATH_LEN];
    if (getcwd(buf, sizeof buf) == NULL) {
        fprintf(stderr, "%s: cd: getcwd: %s\n",
                PROGRAM_NAME, strerror(errno));
        return MISHI_ERR;
    }

    const char *old_pwd = getenv("PWD");
    if (old_pwd != NULL && setenv("OLDPWD", old_pwd, 1) != 0) {
        fprintf(stderr, "%s: cd: setenv: %s\n",
                PROGRAM_NAME, strerror(errno));
        return MISHI_ERR;
    }

    if (setenv("PWD", buf, 1) != 0) {
        fprintf(stderr, "%s: cd: setenv: %s\n",
                PROGRAM_NAME, strerror(errno));
        return MISHI_ERR;
    }

    return MISHI_OK;
}


/* reactiva un procés en el background */
int internal_bg(char **args) {
    (void)args;
    return MISHI_OK;
}

/* atura execució */
int internal_exit(char **args) {
    (void)args;
    printf("adéu!\n");
    exit(0);
}

/* defineix el valor d'una variable d'entorn */
int internal_export(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "%s: export: falten arguments", PROGRAM_NAME);
        return MISHI_ERR;
    }

    /* dins args[1] tenim 'KEY=VALUE' */
	char *key_ptr = strtok(args[1], "=");
	char *value_ptr = strtok(NULL, "=");

	if(value_ptr == NULL) {
		/* no s'ha trobat el '=' */
		fprintf(stderr, "%s: export: expected 'KEY=VALUE' argument syntax\n", PROGRAM_NAME);
		return MISHI_ERR;
	}

	DEBUG_PRINT("args[1]: %s\n", args[1]);
	DEBUG_PRINT("key_ptr: %s\n", key_ptr);
	DEBUG_PRINT("value_ptr: %s\n", value_ptr);

	if (setenv(key_ptr, value_ptr, 1) == -1) {
		fprintf(stderr, "%s: export: %s\n", PROGRAM_NAME,  strerror(errno));
	}

    printf("$%s is now '%s'\n", key_ptr, getenv(key_ptr));
    return MISHI_OK;
}

/* reanuda un procés en el foreground */
int internal_fg(char **args) {
    (void)args;
    return MISHI_OK;
}

/* mostra els processos aturats o en background */
int internal_jobs(char **args) {
    (void)args;
    return MISHI_OK;
}

/* Imprimeix els builtins que tenim */
int internal_help(char **args) {
    (void)args;

    printf("Built-in commands: \n");
    printf("------------------ \n");
    for (size_t i = 0; i < ARRAY_LEN(builtins); i++) {
        printf("%s ", builtins[i].name);
    }
    printf("\n");
    return MISHI_OK;
}

/*
 * Conté el bucle principal del shell
 */
int internal_loop(void) {
    char line[COMMAND_LINE_SIZE];
    int status = MISHI_OK;

    //instanciamos jobs_list y le damos a jobs_list[0] sus valores iniciales
    struct info_job jobs_list[MAX_JOBS]
    jobs_list[0].pid = 0;
    jobs_list[0].estado = 'N';
    //rellena todos los espacios de jobs_list[0].cmd con "\0" hasta su tamaño
    memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd))


    while (1) {
        char *ptr = read_line(line);
        if (ptr == NULL) {

            if (feof(stdin)) {
                internal_exit(NULL);
            }

            fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
            return MISHI_ERR; // fallo del fgets()
        }

        DEBUG_PRINT("[read_line() -> ptr: %s]\n", ptr != NULL ? ptr : "(null)");
        DEBUG_PRINT("[read_line() -> line: %s]\n", line);

        status = execute_line(line);

        if (status == MISHI_ERR) {
            //fprintf(stderr, "%s: hi ha hagut un error\n", PROGRAM_NAME);
            //return MISHI_ERR; /* no crec que faci falta sortir si cd falla */
        }
    }

    return MISHI_OK;
}

/* executa línies de fitxers */
int internal_source(char **args) {
    /* comprobamos la linea*/
    if ((args[1] == NULL) || (args[2] != NULL)) {
        fprintf(stderr, "ERROR SINTAXIS \n source: source [FILE]");
        return MISHI_ERR;
    }

    /* si hem arribat aquí vol dir que tenim source amb més d'un argument */
    FILE *file = fopen(args[1], "r");
    if (file == NULL){
        fprintf(stderr, "%s: source: %s: %s\n", PROGRAM_NAME, args[1], strerror(errno));
        return MISHI_ERR;
    }
    char line[COMMAND_LINE_SIZE];

    while (fgets(line, COMMAND_LINE_SIZE, file) != NULL) {
        size_t length = strlen(line);
        /* Miramos por si length es solo un salto de linea*/
        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        fflush(file);

        if (strlen(line) > 0) {
            execute_line(line);
        }
    }
    fclose(file);
    return MISHI_OK;
}

/* executa una comanda externa
 * això no entra dins nivel2
 * TODO: afegir a la taula
 * 	 	 | PID | cmd | estat |
 */
int launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork(); /* a partir d'ara tenim 2 processos */

    if (pid == 0) { /* procés fill */
        /* execvp() executa els arguments */
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
            exit(EXIT_FAILURE); /* Si retorna, mata el fill */
        }
    } else if (pid < 0) {
        /* -1 és un error de fork() */
        fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
        return MISHI_ERR;
    } else {
        /* ACTUALIZAMOS ESTADOS DE jobs_list[0]*/

        jobs_list[0].pid = pid;
        jobs_list[0].estado = 'E';

        memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd))
            /*Recorre todos los argumentos del cmd y mete espacios*/
        for (int i = 0; args[i] != NULL; i++) {
            strcat(jobs_list[0].cmd, args[i]);

            if (args[i + 1] != NULL) {
                strcat(jobs_list[0].cmd, " ");
            }
        }

        /* procés pare, només espera que el fill acabi */
        do {
        wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && wpid > 0);

        if (wpid == -1) {
            fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
        }
        /* reseteamos todos los valores de jobs_list[0]*/
        jobs_list[0].pid = 0;
        jobs_list[0].estado = 'N';
        //rellena todos los espacios de jobs_list[0].cmd con "\0" hasta su tamaño
        memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd))
    }

    return MISHI_OK;
}

/* imprimeix el prompt a partir de les variables d'entorn USER i PWD */
/* TODO (realment un nice-to-have): afegir suport per colors */
int print_prompt() {

    /* formatam la part de PWD que quedi guapo sense el $HOME */
    const char *pwd = getenv("PWD");
    const char *user = getenv("USER");
    const char *home = getenv("HOME");

    if (pwd == NULL) pwd = "";
    if (user == NULL) user = "";

    if (home != NULL && strncmp(pwd, home, strlen(home)) == 0) {
        printf("[%s :: ~%s] $ ", user, pwd + strlen(home));
    } else {
        printf("[%s :: %s] $ ", user, pwd);
    }
    return EXIT_SUCCESS;
}

/*
 * imprimeix el prompt i llegeix una línia de stdin
 *
 * retorna un punter a la línia llegida o NULL si hi ha un error o un EOF.
 */
char *read_line(char *line) {

    print_prompt();

    if (fgets(line, COMMAND_LINE_SIZE, stdin) == NULL) {
        /* si fgets() retorna NULL hi ha hagut un error o EOF */

        printf("%s: %s\n", PROGRAM_NAME, strerror(errno));
        return NULL;
    }

    strip_newline(line);
    return line;
}

/*
 * li passam la línia llegida acabada amb \0 `line`
 * i la llista de strings `args` a ser emplenada
 *
 * line es destrueix
 */
int parse_args(char *line, char **args) {

    char *token = strtok(line, TOKEN_DELIMITER);
    int idx = 0;

    while (token != NULL && idx < ARGS_SIZE - 1) {

        if (token[0] == '#') {
            /* Si *comença* amb '#' és un comentari */
            break;
        }

        args[idx++] = token;

        /* strtok() guarda estat, guarda el seg char després del split token, x
         * això li passam NULL */
        token = strtok(NULL, TOKEN_DELIMITER);
    }

    args[idx] = NULL;
    return idx;
}


/* sobreescriu el \n del final de `line` amb un '\0'*/
void strip_newline(char *line) { line[strcspn(line, "\n")] = '\0'; }
