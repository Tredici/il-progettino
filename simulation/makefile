# Questo makefile serve ad avviare peer e ds nella cartella corrente
# e a generare i file per i test

CC=gcc
LDLIBS=-lm

# genera i file per i test
test:

generate-register: generate-register.c ../commons.h ../commons.c ../time_utils.c ../time_utils.h ../register.c ../register.h ../set.c ../set.h ../rb_tree.c ../rb_tree.h ../list.c ../list.h

# rimuove dalla cartella corrente i file dei registri presenti
clear:
	rm *.txt
