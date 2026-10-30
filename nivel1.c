#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int execute(char **args);
int handle_line(char *line);
int launch(char **args);
int mishi_loop();
int print_prompt();
int read_line(char *line);
void split_line(char *line, char **args);
void strip_newline(char *line);

/* Builtins */
// int mishi_bg(char **args);
int mishi_cd(char **args);
int mishi_exit(char **args);
// int mishi_export(char **args);
// int mishi_fg(char **args);
int mishi_help(char **args); // no feia falta ficar-la però era fàcil d afegir
// int mishi_jobs(char **args);
// int mishi_source(char **args);

struct builtin builtins[] = {
    { "cd"     , mishi_cd},
    { "exit"   , mishi_exit},
    // { "export" , mishi_export },
    // { "fg"     , mishi_fg },
    { "help"   , mishi_help},
    // { "jobs"   , mishi_jobs },
    // { "source" , mishi_source },
};

/* A partir de l'array retorna nombre de builtins */
#define NUM_BUILTINS (sizeof(builtins) / sizeof(builtins[0]))

int main(void) { return mishi_loop(); }

int execute(char **args) {
    /* mirar usuari ha passat un command buit */
    if (args == NULL || args[0] == NULL) { /* short-circuit ben guapo */
        return 1;
    }

    /* primer mirar si és algun dels nostres builtins */
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            return builtins[i].func(args);
        }
    }

    return launch(args);
}

/* WARNING: Aquesta funció en teoria és `execute_line()` però no m ha agradat es nom
 *
 * S'hi passa una línea sencera acabada amb \0.
 * line es destrueix i es divideix en una llista de strings (args)
 * S'executa la llista de strings (args)
 * retorna el valor de retorn de la comanda executada.
 */
int handle_line(char *line) {
    char *args[ARGS_SIZE] = {0};
    split_line(line, args);

#ifdef DEBUG
    for (int i = 0; args[i] != NULL; i++) {
        printf("args[%d]: '%s'\n", i, args[i]);
    }
#endif // DEBUG

    return execute(args);
}

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
        /* procés pare, només espera que el fill acabi */
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status) && wpid > 0);

        if (wpid == -1) {
            fprintf(stderr, "%s: %s\n", PROGRAM_NAME, strerror(errno));
        }
    }

    return MISHI_OK;
}

/* reb llista de strings amb els arguments de cd i canvia el directori
 * TODO: no actualitza PWD. */
int mishi_cd(char **args) {
    if (args[1] == NULL) {
        /* cas que ens passen només un param (`cd` tot sol) */
        /* més endavant, jo faria que això vagi al $HOME (posix) */
        // fprintf(stderr, "%s: expected argument to \"cd\"\n", PROGRAM_NAME);
        // return MISHI_ERR;
        if (chdir(getenv("HOME")) != 0) {
			goto chdir_error;
        }
		goto update_pwds;
    }

    /* chdir no actualitza PWD.
     * si no hi ha errors, s'actualitzen PWD i OLDPWD */
    if (strlen(args[1]) == 1 && args[1][0] == '-') {
        /* l'usuari ha fet `cd -`, fem cd al OLDPWD */
        if (chdir(getenv("OLDPWD")) != 0) {
			goto chdir_error;
        }
    } else if (chdir(args[1]) != 0) {
		goto chdir_error;
    }

update_pwds:
    setenv("OLDPWD", getenv("PWD"), 1); /* sobreescriure OLDPWD amb PWD */
    char buf[MAX_PATH_LEN] = {0};
    getcwd(buf, MAX_PATH_LEN);
    setenv("PWD", buf, 1); /* actualitzar PWD amb el nou */
    return MISHI_OK;

chdir_error:
	fprintf(stderr, "%s: cd: %s: %s\n", PROGRAM_NAME, args[1], strerror(errno));
	return MISHI_ERR;
}

/* atura execució */
int mishi_exit(char **args) {
    (void)args;
    exit(0);
}

/* Imprimeix els builtins que tenim */
int mishi_help(char **args) {
    (void)args;

    printf("Built-in commands: \n");
    printf("------------------ \n");
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        printf("%s ", builtins[i].name);
    }
    printf("\n");
    return MISHI_OK;
}

int mishi_loop(void) {
    char line[COMMAND_LINE_SIZE];
    int status = MISHI_OK;

    while (1) {
        print_prompt();
        status = read_line(line);
        if (status == MISHI_EOF) {
            printf("\n");
            mishi_exit(NULL);
        }

        strip_newline(line);

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

        if (status == MISHI_ERR) {
            fprintf(stderr, "%s: hi ha hagut un error\n", PROGRAM_NAME);
            return EXIT_FAILURE;
        }
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
 * llegeix una línia de l'usuari i la fica dins line
 *
 * teòricament read_line tb imprimeix prompt però no m ha agradat
 * imprimeix el prompt i llegeix una línia de stdin
 *
 * TODO: Super cursed que la funció 'read_line()' mostri el prompt
 * retorna:
 * 		0 si és un EOF
 * 		1 si tot ha anat correcte
 * 		-1 si hi ha hagut error
 */
int read_line(char *line) {
    if (NULL == fgets(line, COMMAND_LINE_SIZE, stdin)) {
        /* si fgets() retorna NULL hi ha hagut un error */
        if (feof(stdin)) {
            /* comprovar si s'ha tancat stdin amb C-d (feof retornaria nonzero) */
            return MISHI_EOF;
        }
        printf("%s: %s\n", PROGRAM_NAME, strerror(errno));
        return MISHI_ERR;
    }

    return MISHI_OK;
}

/* li passam la línia llegida acabada amb \0 `line`
 * i la llista de strings `args` a ser emplenada */
void split_line(char *line, char **args) {

    char *token = strtok(line, TOKEN_DELIMITER);
    int idx = 0;

    while (token != NULL && idx < ARGS_SIZE - 1) {

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

/* sobreescriure el \n del final (que deixa fgets()) amb '\0' */
void strip_newline(char *line) {
    // size_t len = strlen(line);
    // if (len > 0 && line[len - 1] == '\n') {
    //     line[len - 1] = '\0';
    // }
    line[strcspn(line, "\n")] = '\0';
}
