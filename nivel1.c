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
char *read_line(char *line);
int handle_line(char *line);
void split_line(char *line, char **args);
int execute(char **args);
int mishi_num_builtins();

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


int execute(char **args) {
    /* mirar usuari ha passat un command buit */
    if (args == NULL || args[0] == NULL) { /* short-circuit ben guapo */
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
     * line es destrueix i es divideix en una llista de strings (args)
     * S'executa la llista de strings (args)
     */

    char *args[ARGS_SIZE];
    split_line(line, args);
    execute(args);

#ifdef DEBUG
    for (int i = 0; args[i] != NULL; i++) {
        printf("args[%d]: '%s'\n", i, args[i]);
    }
#endif // DEBUG

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
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
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

int mishi_loop(void) {
    char line[COMMAND_LINE_SIZE];

    while (1) {
        print_prompt();
        read_line(line);

        /* sobreescriure el \n del final (que deixa fgets()) amb '\0' */
        int lgth = strlen(line);
        if (lgth > 0 && line[lgth - 1] == '\n') {
            line[lgth - 1] = '\0';
        }

#ifdef DEBUG
        printf("line: '%s'\n", line);
#endif // DEBUG

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


/* A partir de l'array de strings (noms de builtins),
 * retorna nombre de builtins */
inline int mishi_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}


/* imprimeix el prompt a partir de les variables d'entorn USER i PWD */
/* TODO (realment un nice-to-have): afegir suport per colors */
int print_prompt() {

    /* formatam la part de PWD que quedi guapo sense el $HOME */
    char *pwd = getenv("PWD");
    char *user = getenv("USER");
    char *home = getenv("HOME");

    if (pwd == NULL) pwd = "";
    if (user == NULL) user = "";
    if (home == NULL) home = "";

    if (strlen(home) > 0 && strncmp(pwd, home, strlen(home)) == 0) {
		printf("[%s :: ~%s] $ ", user, pwd+strlen(home));
    } else {
		printf("[%s :: %s] $ ", user, pwd);
	}
    return EXIT_SUCCESS;
}

char *read_line(char *line) {
    /*
     * llegeix una línia de l'usuari i la fica dins line
     *
     * teòricament read_line tb imprimeix prompt però no m ha agradat
     * imprimeix el prompt i llegeix una línia de stdin
     *
     * TODO: Super cursed que la funció 'read_line()' mostri el prompt
     */
    if (NULL == fgets(line, COMMAND_LINE_SIZE, stdin)) {
        /* si fgets() retorna NULL hi ha hagut un error */
        printf("%s: %s\n", PROGRAM_NAME, strerror(errno));
        mishi_exit(NULL);

    } else if (feof(stdin)) {
        /* comprovar si s'ha tancat stdin amb C-d (feof retornaria nonzero) */
        mishi_exit(NULL);
    }

    return line;
}

void split_line(char *line, char **args) {
    /* li passam la línia llegida acabada amb \0 `line`
     * i la llista de strings `args` a ser emplenada */

    char *token = NULL;
    int idx = 0;

    token = strtok(line, TOKEN_DELIMITER);

    while (token != NULL) {

        if (token[0] == '#') {
            /* Si *comença* amb # és un comentari */
            break;
        }
        args[idx++] = token;

        /* strtok() guarda estat, guarda el seg char després del split token, x
         * eso li passam NULL */
        token = strtok(NULL, TOKEN_DELIMITER); // That's why NULL is passed
    }

    args[idx] = NULL;
}

