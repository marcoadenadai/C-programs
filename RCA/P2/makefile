#MAKEFILE COMPILA PROJETO 2

all: modulo_usuario servidor_central
	
servidor_central: servidor_central.c lib/regs.o
	gcc -o servidor_central servidor_central.c lib/regs.o -pthread

modulo_usuario: lib/contatos.o lib/regs.o lib/ui.o lib/messenger.o modulo_usuario.c
	gcc -o modulo_usuario modulo_usuario.c lib/regs.o lib/contatos.o lib/ui.o lib/messenger.o -pthread -lncurses

lib/messenger.o: lib/messenger.c lib/messenger.h
	gcc -o lib/messenger.o lib/messenger.c -c

lib/ui.o: lib/ui.c lib/ui.h lib/contatos.o
	gcc -o lib/ui.o lib/ui.c -lncurses -c 

lib/contatos.o: lib/contatos.c lib/contatos.h
	gcc -o lib/contatos.o lib/contatos.c -c

lib/regs.o: lib/regs.c lib/regs.h
	gcc -o lib/regs.o lib/regs.c -c

clean: 
	rm lib/*.o *.ctt modulo_usuario servidor_central