`mishi` (minishell). Pràctica 2 de Sistemes Operatius Curs 2026.

> Estudiants: Jaume Cladera, Arnau K. Deprez, Matías Volpe

## Progrés

Estam finalitzant `nivell3.c`

## Instalar i compilar

El [Makefile](Makefile) compila tots els nivells per defecte:

```
git clone https://github.com/arnauKL/mishi
cd mishi
make
```

Per compilar només un nivell concret, cal especificar el target:

```
make nivel3
```


## TODO

- aïllar funcionalitat segons el nivell (al nivell 1 no hauríem de poder executar `ls`, per exemple).

- implementar `cd` avançat (a partir del nivell 2).

- afegir colors per missatges d'error.
