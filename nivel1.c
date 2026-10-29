#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "constants.h"

int mishi_loop();
int print_prompt();
int launch(char **args);
char *read_line();
int handle_line(char *line);
void split_line(char *line, char **args);
int execute(char **args);

/* Builtins */
int mishi_exit(char **args);
int mishi_help(char **args); // no feia falta ficar-la però era fàcil d afegir
int mishi_cd(char **args);
int mishi_export(char **args);
int mishi_source(char **args);
int mishi_jobs(char **args);
int mishi_fg(char **args);
int mishi_bg(char **args);

char *builtin_str[] = {"cd", "help", "exit"};
int (*builtin_func[])(char **) = {&mishi_cd, &mishi_help, &mishi_exit};

int main(void) {
	mishi_loop();
	return EXIT_SUCCESS;
}

int mishi_loop(void) {
    char line[COMMAND_LINE_SIZE];

    while (1) {
        print_prompt();
        read_line();

        // tot lo des do{}while s ha d refactoritzar en una funció read_line o algo
        // ojo: fgets retains newline character.
        do {
            if (NULL == fgets(line, sizeof(line), stdin)) {
                /* si fgets() retorna NULL hi ha hagut un error */
                printf("%s: %s\n", PROGRAM_NAME, strerror(errno));
                mishi_exit(NULL);

            } else if (feof(stdin)) {
                /* comprovar si s'ha tancat stdin amb C-d (feof retornaria nonzero) */
                mishi_exit(NULL);
            }
        } while (0);

        /* sobreescriure el \n del final (que deixa fgets()) amb '\0' */
        int lgth = strlen(line);
        if (lgth > 0 && line[lgth - 1] == '\n') {
            line[lgth - 1] = '\0';
        }

        /*
         * una vegada arribat aquí tenim `char* line` que conté tota la línia
         * llegida per fgets(), p. ex:
         *
         * 		line => "cd directori_prova" (acabat amb \0)
         *
         * Ara s'ha de tractar la `line`.
         *
         * Això en teoria s'ha de fer amb una funció execute_line() però és
         * super cursed que parseja primer i executa després tot dins una
         * funció que se diu execute_line().
         *
         * x eso jo li he dit handle_line().
         */
        handle_line(line);
    }

    return EXIT_SUCCESS;
}

int mishi_num_builtins() {
    /* A partir de l'array de strings (noms de builtins),
     * retorna nombre de builtins */
    return sizeof(builtin_str) / sizeof(char *);
}

void split_line(char *line, char **args) {
    /* li passam la línia llegida acabada amb \0 `line`
     * i la llista de strings `args` a ser emplenada */

    char *token = NULL;
    int idx = 0;

    token = strtok(line, TOKEN_DELIMITER);

    while (token != NULL) {

		if (token[0] == '#') {
			/* En cas que hi hagi un comentari */
			break;
		}
        args[idx++] = token;

        /* strtok() guarda estat, guarda el seg char després del split token, x
         * eso li passam NULL */
        token = strtok(NULL, TOKEN_DELIMITER); // That's why NULL is passed
    }

    args[idx] = NULL;
}

int execute(char **args) {
    if (args == NULL || args[0] == NULL) { /* short-circuit ben guapo */
                                           /* usuari ha passat un command buit */
        return 1;
    }

    for (int i = 0; i < mishi_num_builtins(); i++) {
        /* primer mirar si és algun dels nostres builtins */
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return launch(args);
}

int handle_line(char *line) {
    /* WARNING: Aquesta funció en teoria és `execute_line()` però no m ha agradat es nom
     *
     * S'hi passa una línea sencera acabada amb \0.
     * Es divideix en una llista de strings
     * S'executa
     */

    char *args[ARGS_SIZE];
    split_line(line, args);
    execute(args);

    return EXIT_SUCCESS;
}

int launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    /* a partir d'ara tenim 2 processos */

    if (pid == 0) { /* procés fill */
        /* execvp() executa els arguments */
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
            exit(EXIT_FAILURE); /* Si retorna, mata el fill */
        }
    } else if (pid < 0) {
        /* -1 és un error de fork() */
        fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
    } else {
        /* procés pare, només espera que el fill acabi */
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
            /* TODO: algo d error checking o d veure s exit value del fill */
            if (wpid == 42) {
                wpid++;
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

char *read_line() {
    /*
     * imprimeix el prompt i llegeix una línia de stdin
     *
     * TODO: Super cursed que la funció 'read_line()' mostri el prompt
     */

    // print_prompt();

    return "";
}

int print_prompt() {
    /* imprimeix el prompt a partir de les variables d'entorn USER i PWD */
    /* TODO (realment un nice-to-have): afegir suport per colors */

    /* formatam la part de PWD que quedi guapo sense el $HOME */
    char buf[256];
    strncpy(buf, getenv("PWD"), sizeof(buf));
    char *idx = buf + strlen(getenv("HOME")) + 1;

    printf("[%s :: %s] $ ", getenv("USER"), idx);

    // faltaria comprovació d errors
    return EXIT_SUCCESS;
}

int mishi_cd(char **args) {
    /* reb llista de strings amb els arguments de cd */
    if (args[1] == NULL) {
        /* cas que ens passen només un param (`cd` tot sol) */
        /* més endavant, jo faria que això vagi al $HOME (posix) */
        fprintf(stderr, "%s: expected argument to \"cd\"\n", PROGRAM_NAME);
        return 1;
    }

    /* crec q chdir no actualitza PWD i pentura estaria guapo mirar d q tb s'actualitzi
     * OLDPWD si després tb volem implementar la funcionalitat de `cd -`
     */
    if (chdir(args[1]) != 0) {
        fprintf(stderr, "%s: cd: %s: %s\n", PROGRAM_NAME, args[1], strerror(errno));
    }

    return 1;
}

int mishi_exit(char **args) {
    /* atura execució */
    exit(0);
}

int mishi_help(char **args) {
    /* Imprimeix els builtins que tenim */

    fprintf(stdout, "Built-in commands: \n");
    fprintf(stdout, "------------------ \n");
    for (int i = 0; i < mishi_num_builtins(); i++) {
        fprintf(stdout, "%s ", builtin_str[i]);
    }
    fprintf(stdout, "\n");
    return 1;
}
