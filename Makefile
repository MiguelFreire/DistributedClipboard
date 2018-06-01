CC = gcc
CFLAGS = -Wall -g

all: clipboard many_clients_one_region many_clients_one_region_2 one_clipboard_to_many many_clients_wait
clipboard: clipboard.o clipboard_com.o clipboard_protocol.o clipboard_handler.o clipboard_core.o clipboard_threads.o library.o cblist.o utils.o cbmessage.pb-c.o
			$(CC) $(CFLAGS) -o clipboard clipboard.o clipboard_com.o clipboard_protocol.o clipboard_handler.o clipboard_core.o clipboard_threads.o library.o cblist.o utils.o cbmessage.pb-c.o -lprotobuf-c -lpthread
many_clients_one_region: many_clients_one_region.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o many_clients_one_region many_clients_one_region.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
many_clients_one_region_2: many_clients_one_region_2.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o many_clients_one_region_2 many_clients_one_region_2.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
many_clients_wait: many_clients_wait.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o many_clients_wait many_clients_wait.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
one_clipboard_to_many: one_clipboard_to_many.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o one_clipboard_to_many one_clipboard_to_many.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
clipboard.o: clipboard.c clipboard.h clipboard_threads.h cblist.h
			$(CC) $(CFLAGS) -c clipboard.c
clipboard_threads.o: clipboard_threads.c clipboard_threads.h
			$(CC) $(CFLAGS) -c clipboard_threads.c
clipboard_core.o: clipboard_core.c clipboard_core.h 
			$(CC) $(CFLAGS) -c clipboard_core.c
clipboard_handler.o: clipboard_handler.c clipboard_handler.h clipboard_core.h
			$(CC) $(CFLAGS) -c clipboard_handler.c
clipboard_protocol.o: clipboard_protocol.c clipboard_protocol.h
			$(CC) $(CFLAGS) -c clipboard_protocol.c
clipboard_com.o: clipboard_com.c clipboard_com.h utils.h
			$(CC) $(CFLAGS) -c clipboard_com.c
library.o: library.c clipboard.h cbmessage.pb-c.h clipboard_protocol.h
			$(CC) $(CFLAGS) -c library.c
many_clients_one_clipboard.o: many_clients_one_region.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_one_region.c 
many_clients_one_region_2.o: many_clients_one_region_2.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_one_region_2.c 
one_clipboard_to_many.o: one_clipboard_to_many.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c one_clipboard_to_many.c
many_clients_wait.o: many_clients_wait.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_wait.c 
utils.o:  utils.c utils.h
			$(CC) $(CFLAGS) -c utils.c
cbmessage.pb-c.o: cbmessage.pb-c.c cbmessage.pb-c.h
			$(CC) $(CFLAGS) -c cbmessage.pb-c.c 
cblist.o: cblist.c cblist.h utils.h
			$(CC) $(CFLAGS) -c cblist.c 
clean:
			rm -rf *.o
			rm clipboard
proto:
			protoc --c_out=. cbmessage.proto	