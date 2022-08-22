all: dmm scope

dmm: dmm.c
	$(CC) -o dmm dmm.c -lgd

scope: scope.c
	$(CC) -o scope scope.c -ltenma -lusb-1.0 -lgd -ggdb3

