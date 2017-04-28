all: client server

client:
	gcc -Wall -o netclient netclient.c libnetfiles.c

server:
	gcc -Wall -pthread -o server netfileserver.c

test: server
	gcc -Wall -o client ClientRunner.c libnetfiles.c

ctest:
	rm -f client server

clean:
	rm -f netclient server
