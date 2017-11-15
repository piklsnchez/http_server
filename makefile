IDIR=include
LIBS=-lnettle
CFLAGS=-I$(IDIR) $(LIBS)
OBJ=http_server.o websocket.o socket.o hash_map.o

http_server : $(OBJ)
	gcc $(CFLAGS) -o http_server main.c $(OBJ) -ggdb -Wall
http_server.o :
	gcc $(CFLAGS) -c http_server.c -ggdb -Wall
websocket.o :
	gcc $(CFLAGS) -c websocket.c -ggdb -Wall
socket.o :
	gcc $(CFLAGS) -c socket.c -ggdb -Wall
hash_map.o :
	gcc $(CFLAGS) -c hash_map.c -ggdb -Wall

.PHONY : clean
clean : 
	rm -f http_server $(OBJ)
