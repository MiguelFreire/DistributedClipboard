CC = gcc
CFLAGS = -Wall -g

all: clipboard client

clipboard: clipboard.o utils.o library.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o clipboard clipboard.o utils.o cbmessage.pb-c.o library.o -lprotobuf-c
client: client.o utils.o library.o cbmessage.pb-c.o 
			$(CC) $(CFLAGS) -o client client.o utils.o library.o cbmessage.pb-c.o -lprotobuf-c
clipboard.o: clipboard.c clipboard.h utils.h cbmessage.pb-c.h
			$(CC) $(CFLAGS) -c clipboard.c
client.o: client.c clipboard.h utils.h
			$(CC) $(CFLAGS) -c client.c 
utils.o:  utils.c utils.h
			$(CC) $(CFLAGS) -c utils.c
cbmessage.pb-c.o: cbmessage.pb-c.c cbmessage.pb-c.h
			$(CC) $(CFLAGS) -c cbmessage.pb-c.c 
library.o: library.c clipboard.h cbmessage.pb-c.h
			$(CC) $(CFLAGS) -c library.c 

clean:
			rm -rf *.o
			rm clipboard
			rm client
			rm CLIPBOARD_SOCKET
			rm APP_SOCKET_*
proto:
			protoc --c_out=. cbmessage.proto	