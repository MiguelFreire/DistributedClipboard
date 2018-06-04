CC = gcc
CFLAGS = -Wall -g

all: clipboard app1 app2 app3 app4
clipboard: clipboard.o clipboard_com.o clipboard_protocol.o clipboard_handler.o clipboard_core.o clipboard_threads.o library.o cblist.o utils.o cbmessage.pb-c.o
			$(CC) $(CFLAGS) -o clipboard clipboard.o clipboard_com.o clipboard_protocol.o clipboard_handler.o clipboard_core.o clipboard_threads.o library.o cblist.o utils.o cbmessage.pb-c.o -lprotobuf-c -lpthread
app1: many_clients_one_region.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o app1 many_clients_one_region.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
app2: many_clients_one_region_2.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o app2 many_clients_one_region_2.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
app3: many_clients_wait.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o app3 many_clients_wait.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
app4: one_clipboard_to_many.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o app4 one_clipboard_to_many.o library.o utils.o clipboard_protocol.o cbmessage.pb-c.o -lprotobuf-c
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
many_clients_one_region.o: many_clients_one_region.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_one_region.c 
many_clients_one_region_2.o: many_clients_one_region_2.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_one_region_2.c 
one_clipboard_to_many.o: one_clipboard_to_many.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c one_clipboard_to_many.c
many_clients_wait.o: many_clients_wait.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_wait.c
many_clients_many_regions.o: many_clients_many_regions.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c many_clients_many_regions.c 
utils.o:  utils.c utils.h
			$(CC) $(CFLAGS) -c utils.c
cbmessage.pb-c.o: cbmessage.pb-c.c cbmessage.pb-c.h
			$(CC) $(CFLAGS) -c cbmessage.pb-c.c 
cblist.o: cblist.c cblist.h utils.h
			$(CC) $(CFLAGS) -c cblist.c 
clean:
			rm -rf *.o
			rm clipboard
			rm app1
			rm app2
			rm app3
			rm app4
proto:
			protoc --c_out=. cbmessage.proto
install:
			git clone https://github.com/protobuf-c/protobuf-c.git
			./autogen.sh && ./configure && make && make install