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


