# Adaptat del Makefile de n'Adelaida

# Comenta per eliminar missatges de debug de tots els nivells
DEBUG = -DDEBUG 

CC=gcc
CFLAGS=-c -g -Wall -Wextra -Wpedantic -std=c17 $(DEBUG)

SOURCES=nivel1.c nivel2.c nivel3.c nivel4.c nivel5.c #nivel6.c my_shell.c
LIBRARIES= #.o
INCLUDES= #.h
PROGRAMS=nivel1 nivel2 nivel3 nivel4 nivel5 #nivel6 my_shell
OBJS=$(SOURCES:.c=.o)
NOM_CARPETA=cladera_deprez_volpe

all: $(OBJS) $(PROGRAMS) #misleep contador

#$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
#	$(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

nivel1: nivel1.o
	$(CC) $@.o -o $@ $(LIBRARIES)

nivel2: nivel2.o
	$(CC) $@.o -o $@ $(LIBRARIES)

nivel3: nivel3.o
	$(CC) $@.o -o $@ $(LIBRARIES)

nivel4: nivel4.o
	$(CC) $@.o -o $@ $(LIBRARIES)

nivel5: nivel5.o
	$(CC) $@.o -o $@ $(LIBRARIES)

#nivel6: nivel6.o
#	$(CC) $@.o -o $@ $(LIBRARIES)

#my_shell: my_shell.o
#	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

# TODO: per facilitar entrega
entrega: 
	mkdir -p $(NOM_CARPETA)
	cp nivel*.c $(NOM_CARPETA)/
	tar -czvf $(NOM_CARPETA).tar.gz $(NOM_CARPETA)

.PHONY: clean
clean:
	rm -rf *.o *~ *.tmp $(PROGRAMS) $(NOM_CARPETA)
