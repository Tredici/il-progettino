CFLAGS=-Wall -g -pthread -D_RB_TREE_DEBUG -D_LIST_DEBUG -Wextra -rdynamic
CC=gcc
# librerie che il linker dovrà richiamare
# vedi: http://retis.santannapisa.it/luca/makefiles.pdf
LDLIBS=-lm

default:
	echo "Speficificare una regola"

test_socket: test_socket.c ../socket_utils.h ../socket_utils.c


socket: test_socket
	./test_socket

queue.o: ../queue.h ../queue.c

test_queue: test_queue.c ../queue.h ../queue.c

queue: test_queue
	./test_queue

test_rb_tree: test_rb_tree.c ../rb_tree.h ../rb_tree.c

rb_tree: test_rb_tree
	./test_rb_tree

crash_rb_tree: crash_rb_tree.c ../rb_tree.h ../rb_tree.c

crash: crash_rb_tree
	./crash_rb_tree

test_entry_parse: test_entry_parse.c ../register.h ../register.c ../list.h ../list.c

entry_parse: test_entry_parse
	./test_entry_parse

test_repl: test_repl.c ../repl.c ../repl.h

repl: test_repl
	./test_repl

test_repl_cmd: test_repl_cmd.c ../repl.c ../repl.h

repl_cmd: test_repl_cmd
	./test_repl_cmd

test_repl_loop: test_repl_loop.c ../repl.c ../repl.h

repl_loop: test_repl_loop
	./test_repl_loop

test_list_accumulate: test_list_accumulate ../list.h ../list.c

list_accumulate: test_list_accumulate
	./test_list_accumulate

test_list_eliminate: test_list_eliminate.c ../list.h ../list.c

list_eliminate: test_list_eliminate
	./test_list_eliminate


test_list_map: test_list_map.c ../list.h ../list.c
list_map: test_list_map
	./test_list_map

test_list_reduce: test_list_reduce.c ../list.h ../list.c
list_reduce: test_list_reduce
	./test_list_reduce

test_list_select: test_list_select.c ../list.h ../list.c ../commons.h ../commons.c
list_select: test_list_select
	./test_list_select

test_thread_semaphore: test_thread_semaphore.c ../thread_semaphore.h ../thread_semaphore.c

thread_semaphore: test_thread_semaphore
	./test_thread_semaphore

test_long_life_thread: test_long_life_thread.c ../thread_semaphore.h ../thread_semaphore.c

long_life_thread: test_long_life_thread
	./test_long_life_thread

test_errExit: test_errExit.c ../commons.h ../commons.c

errExit: test_errExit
	./test_errExit

test_rb_tree_next_prev: test_rb_tree_next_prev.c ../rb_tree.h ../rb_tree.c

rb_tree_next_prev: test_rb_tree_next_prev
	./test_rb_tree_next_prev

test_rb_tree_foreach: test_rb_tree_foreach.c ../rb_tree.h ../rb_tree.c

rb_tree_foreach: test_rb_tree_foreach
	./test_rb_tree_foreach

test_rb_tree_accumulate: test_rb_tree_accumulate.c ../rb_tree.h ../rb_tree.c

rb_tree_accumulate: test_rb_tree_accumulate
	./test_rb_tree_accumulate

rb_tree.o: ../rb_tree.h ../rb_tree.c

test_set: test_set.c ../set.h ../set.c ../rb_tree.h ../rb_tree.c

set: test_set
	./test_set

test_set_foreach: test_set_foreach.c ../set.h ../set.c ../rb_tree.h ../rb_tree.c

set_foreach: test_set_foreach
	./test_set_foreach

test_waitForInput: test_waitForInput.c ../commons.h ../commons.c
waitForInput: test_waitForInput
	./test_waitForInput

test_waitForInputPolling: test_waitForInputPolling.c ../commons.h ../commons.c
waitForInputPolling: test_waitForInputPolling
	./test_waitForInputPolling

#testa fatal
test_fatal: test_fatal.c ../commons.h ../commons.c

fatal: test_fatal
	./test_fatal

#unified_io.o: ../unified_io.c ../unified_io.h
#test_unified_io.o: test_unified_io.c

#test_unified_io: test_unified_io.o unified_io.o
test_unified_io: test_unified_io.c ../unified_io.c ../unified_io.h ../queue.h ../queue.c ../commons.h ../commons.c
	gcc $(CFLAGS) -o test_unified_io test_unified_io.c ../unified_io.c ../unified_io.h ../queue.h ../queue.c ../commons.h ../commons.c

unified_io: test_unified_io
	./test_unified_io

test_unified_io_sync: test_unified_io_sync.c ../unified_io.c ../unified_io.h ../queue.h ../queue.c ../commons.h ../commons.c
	gcc $(CFLAGS) -o test_unified_io_sync test_unified_io_sync.c ../unified_io.c ../unified_io.h ../queue.h ../queue.c ../commons.h ../commons.c

unified_io_sync: test_unified_io_sync
	./test_unified_io_sync

test_main_loop: test_main_loop.c ../main_loop.h ../main_loop.c ../repl.h ../repl.c ../unified_io.c ../unified_io.h ../queue.h ../queue.c ../commons.h ../commons.c
	gcc $(CFLAGS) -o test_main_loop test_main_loop.c ../main_loop.h ../main_loop.c ../repl.h ../repl.c ../unified_io.c ../unified_io.h ../queue.h ../queue.c ../commons.h ../commons.c

main_loop: test_main_loop
	./test_main_loop

test_argParseRange: test_argParseRange.c ../commons.h ../commons.c

argParseRange: test_argParseRange
	./test_argParseRange

test_ns_host_addr: test_ns_host_addr.c ../commons.h ../commons.c ../ns_host_addr.h ../ns_host_addr.c ../socket_utils.h ../socket_utils.c

ns_host_addr: test_ns_host_addr
	./test_ns_host_addr

test_udp_listener: test_udp_listener.c ../commons.h ../commons.c ../socket_utils.h ../socket_utils.c ../messages.h ../messages.c ../ns_host_addr.h ../ns_host_addr.c

udp_listener: test_udp_listener
	./test_udp_listener

test_tcp: test_tcp.c ../commons.h ../commons.c ../socket_utils.h ../socket_utils.c

tcp: test_tcp
	./test_tcp

test_send_shutdown_req: test_send_shutdown_req.c ../commons.h ../commons.c ../socket_utils.h ../socket_utils.c ../messages.h ../messages.c ../ns_host_addr.h ../ns_host_addr.c

test_time_utils: test_time_utils.c ../time_utils.h ../time_utils.c ../commons.h ../commons.c

time_utils: test_time_utils
	./test_time_utils

# riusciamo a eseguire il parsing delle date?
test_parse_date: test_parse_date.c ../time_utils.h ../time_utils.c ../commons.h ../commons.c

parse_date: test_parse_date
	./test_parse_date

# test con i registri
test_register: test_register.c ../register.h ../register.c ../list.h ../list.c ../set.h ../set.c ../rb_tree.h ../rb_tree.c ../time_utils.h ../time_utils.c ../commons.h ../commons.c

register: test_register
	./test_register

# test per i messaggi di test
test_check: test_check.c ../commons.h ../commons.c ../socket_utils.h ../socket_utils.c ../messages.h ../messages.c ../ns_host_addr.h ../ns_host_addr.c

clean:
	rm -rf *.o

