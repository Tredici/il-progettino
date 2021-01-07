CFLAGS=-Wall -g -pthread -D_RB_TREE_DEBUG -D_LIST_DEBUG -Wextra
CC=gcc


default:
	echo "Speficificare una regola"

all: peer
	./peer


#copiata dal .pdf di Abeni su make all'indirizzo:
#	http://retis.santannapisa.it/luca/makefiles.pdf
#da man gcc si può leggere che l'argomento -MM fa
# generare a gcc una regola make di prerequisiti
# per compilare i file specificati in seguito
#-MF FILE invece fa si che l'output di tale regola
# sia scritto entro FILE 
%.d : %.c
	$(CC) -MM -MF $@ $<
-include $(OBJS:%.o=%.d)

# file esclusivi dei peer
peer_add.o: peer/peer_add.c  peer/peer_add.h
peer_stop.o: peer/peer_stop.c peer/peer_stop.h

# file per tutti
list.o: 			list.h list.c
register.o: 		register.h register.c
repl.o: 			repl.h repl.c
socket_utils.o: 	socket_utils.h socket_utils.c
queue.o: 			queue.h queue.c
main_loop.o: 		main_loop.h main_loop.c
rb_tree.o:			rb_tree.h rb_tree.c
set.o:				set.h set.c rb_tree.h rb_tree.c
commons.o:			commons.h commons.c
thread_semaphore.o:	thread_semaphore.h thread_semaphore.c

# dipendenze del peer
PEERDEPS = list.o register.o repl.o socket_utils.o queue.o main_loop.o rb_tree.o set.o commons.o thread_semaphore.o

# main dei peer
peer.o: peer.c

p2p: peer.o $(PEERDEPS)

clean:
	rm *.o
