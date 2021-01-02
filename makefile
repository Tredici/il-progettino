CFLAGS=-Wall -g -pthread -D_RB_TREE_DEBUG -D_LIST_DEBUG -Wextra

default:
	echo "Speficificare una regola"

all: peer
	./peer

list.o: list.c list.h

register.o: register.c register.h

repl.o: repl.c repl.h

peer_add.o: peer/peer_add.c  peer/peer_add.h

peer_stop.o: peer/peer_stop.c peer/peer_stop.h

peer.o: peer.c

peer: list.o peer.c register.o repl.o peer_add.o peer_stop.o
