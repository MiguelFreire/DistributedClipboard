all: clipboard

clipboard:	local.o utils.o
			gcc -o clipboard local.o utils.o
local.o: local.c clipboard.h utils.o
			gcc -c local.c -Wall
utils.o: utils.c utils.h
			gcc -c utils.c -Wall
clean:
			rm -rf *.o
			rm clipboard