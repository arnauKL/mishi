# notes de classe

## Classe de dijous 12 de novembre.

Nivell 4

hem de crear manejadors especials pels senyals `SIGINT` (C-c) i per SIGCHLD (senyal intern de parada/sortida d'un procés fill).

El pare és qui cridarà el handler de SIGING (C-c).


# Classe dijous 19

Ara, enlloc de bloquejar, podem afegir un ampersand al final `&`.

```sh
sleep 20 &
```

args(darrer) = '&';

Aquest ampersand no ha de passar al `execvp()`. L'hem de substituir per NULL.

execute_line() canvia de funcionament

En el cas del fill evitarem SIGSTP.

De moment tenim SIGINT i SIGCHLD.

Si fem `sleep 20` i aturam el procés quan duu 10 segons, n'hi quedaran 10 més.

Ara mateix, quan executam, el procés pare posa el procés en FG i espera amb `pause()`. Ara hem d'afegir la manera d'afegir al BG.

Això ho farem amb una nova funció `add()` per afegir a la taula

add rebrà PID, estat i cmd. Tendrem una nova variable global `n_job` inicialitzada a 0.
add() anirà a la taula de jobs i afegirà al primer espai que trobi lliure (mai al 0)
IMPORTANT: el nom del cmd dins la taula ha de tenir el `&` tot i que l'hem tret de execvp() abans. Així podrem distinguir processos en FG i BG a la taula.

| PID | Estat | cmd |
|-----|-------|-----|



# Classe dijous 26 de novembre

Ha dit q d lo q explica avui poden entrar coses a s'examen. No només codi del minishell.

Al nivell 5 tenim la taula jobs_lists on el primer procés (0) és el procés en
foreground i els altres, en BG.

| PID | Estat |    cmd     |
|-----|-------|------------|
| 100 |   E   | sleep 20   | < aquest està en fg (índex 0)
|-----|-------|------------|
| 102 |   E   | sleep 40 & | < aquest està en bg

Quan fem C-z, hem d'eliminar l'entrada original i re-afegir-la com a 'D' (detingut) enloc de 'E'.

Al nivell 6 ens apareixen les comandes `fg` i `bg`.

## `fg`

### Pujar un procés que està en Background (s'ha executat amb `&`):

Hem de poder pujar adalt de la taula un procés que està en bg (darrera).

| PID | Estat |    cmd     |
|-----|-------|------------|
|     |       |            | < no hi ha res en fg
|-----|-------|------------|
| 102 |   E   | sleep 40 & | < aquest està en bg

Movem el procés al fg:

| PID | Estat |    cmd   |
|-----|-------|----------|
| 102 |   E   | sleep 40 | < aquest està en fg
|     |       |          | 

I tendrem el procés sense el `&`.
Ara, només hem d'esperar activament a que el procés acabi (el procés ja s'està executant).

### Passar al fg un procés que està detingut ('D')

| PID | Estat |    cmd     |
|-----|-------|------------|
|     |       |            | < no hi ha res en fg
|-----|-------|------------|
| 102 |   D   | sleep 40 & | < aquest està detingut

Movem el procés al fg:

| PID | Estat |    cmd   |
|-----|-------|----------|
| 102 |   E   | sleep 40 | < aquest està en fg
|     |       |          | 

Aquí, el procés ha de passar de Detingut ('D') a Executant-se ('E').
Abans d'esperar activament, hem d'enviar-li un senyal al procés per fer que
reanudi la seva execució.

## el `bg`

Passar de processos aturats (en bg) a processos executant-se (en bg).

Tenim 3 tipus de processos:
    - processos en `fg` (actius)
    - *processos en `bg` detinguts*
    - processos en `bg` executant-se

Els únics processos que podem tocar amb la comanda `bg` són els que estan ja en
`bg` detinguts.

| PID | Estat |    cmd     |
|-----|-------|------------|
|     |       |            |
|-----|-------|------------|
| 102 |   D   | sleep 40   | < aquest està detingut
| 103 |   E   | sleep 25 & | < aquest està executant-se

si fem `bg` al procés que està detingut, tendrem

| PID | Estat |    cmd     |
|-----|-------|------------|
|     |       |            |
|-----|-------|------------|
| 102 |   E   | sleep 40 & | < aquest ara tb està executant-se
| 103 |   E   | sleep 25 & | < aquest està executant-se


> L'ampersand (`&`) l'hem de tornar a afegir quan la comanda passa a estar en
> estat 'E', xq quan l'hem detingut, el procés estava en foreground i no tenia
> el `&`.


## redirecció de stdout (`>`)

L'operador `>` redirigeix l'output del programa de stdout al fitxer `fitxer.txt`.

```sh
ls -l > fitxer.txt
```

Els continguts de ls -l estan al fitxer:

```
$ cat fitxer.txt
total 412
16 -rwxr-xr-x 1 ak ak 16216 Nov 24 10:19 contador
 4 -rw-r--r-- 1 ak ak   352 Nov 24 10:18 contador.c
 0 -rw-r--r-- 1 ak ak     0 Nov 26 11:35 fitxer.txt
 4 -rw-r--r-- 1 ak ak  1034 Nov 24 14:26 Makefile
 ...
```

Detectam que tenim el símbol i el canviam per un `NULL`.

Només farem redirecció a comandes externes.

obrim el *file descriptor* del fitxer amb `open()` i el passam a `dup()`. El
*file descriptor* de STDOUT és 1 (és una macro ja definida dins stdlib.h).
