all: clipboard client

clipboard:	clipboard.o utils.o
			gcc -o clipboard clipboard.o utils.o
client:     client.o utils.o library.o
			gcc -o client client.o utils.o library.o
client.o:	client.c clipboard.h utils.o
			gcc -c client.c -Wall
clipboard.o: clipboard.c clipboard.h utils.o
			gcc -c clipboard.c -Wall
utils.o: 	utils.c utils.h
			gcc -c utils.c -Wall
library.o:  library.c clipboard.h
			gcc -c library.c -Wall
clean:
			rm -rf *.o
			rm clipboard
			rm client
			rm CLIPBOARD_SOCKET
			rm APP_SOCKET_*
			