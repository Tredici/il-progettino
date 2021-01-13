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
ds_peers.o: ds-src/ds_peers.c  ds-src/ds_peers.h
	$(CC) $(CFLAGS) -c -o $@ $<

ds_udp.o: ds-src/ds_udp.c  ds-src/ds_udp.h
	$(CC) $(CFLAGS) -c -o $@ $<

ds_esc.o: ds-src/ds_esc.c ds-src/ds_esc.h
	$(CC) $(CFLAGS) -c -o $@ $<

DSDEPS = ds_esc.o ds_udp.o ds_peers.o


# file esclusivi dei peer
peer_start.o: peer-src/peer_start.c  peer-src/peer_start.h
	$(CC) $(CFLAGS) -c -o $@ $<

peer_udp.o: peer-src/peer_udp.c  peer-src/peer_udp.h
	$(CC) $(CFLAGS) -c -o $@ $<

peer_add.o: peer-src/peer_add.c  peer-src/peer_add.h
	$(CC) $(CFLAGS) -c -o $@ $<

peer_stop.o: peer-src/peer_stop.c peer-src/peer_stop.h
	$(CC) $(CFLAGS) -c -o $@ $<

PEERDEPS = peer_stop.o peer_add.o peer_udp.o peer_start.o

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
unified_io.o:		unified_io.h unified_io.c
ns_host_addr.o:		ns_host_addr.h ns_host_addr.c
messages.o:			messages.h messages.c

# dipendenze del peer
COMMONDEPS = list.o register.o repl.o socket_utils.o queue.o main_loop.o rb_tree.o set.o commons.o thread_semaphore.o unified_io.o ns_host_addr.o messages.o

# main dei peer
peer.o: peer.c

peer: peer.o $(COMMONDEPS) $(PEERDEPS)
	$(CC) $(CFLAGS) -o $@ peer.o $(COMMONDEPS) $(PEERDEPS)

# main del discovery server
ds.o: ds.c

ds: ds.o $(COMMONDEPS) $(DSDEPS)
	$(CC) $(CFLAGS) -o $@ ds.o $(COMMONDEPS) $(DSDEPS)

clean:
	rm *.o
