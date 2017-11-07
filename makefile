IDIR=include
CFLAGS=-I$(IDIR)
OBJ=http_server.o socket.o hash_map.o

http_server : $(OBJ)
	gcc $(CFLAGS) -o http_server main.c $(OBJ) -ggdb -Wall
http_server.o :
	gcc $(CFLAGS) -c http_server.c -ggdb -Wall
socket.o :
	gcc $(CFLAGS) -c socket.c -ggdb -Wall
hash_map.o :
	gcc $(CFLAGS) -c hash_map.c -ggdb -Wall

.PHONY : clean
clean : 
	rm -f http_server $(OBJ)
