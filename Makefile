all: client server

client:
	gcc -Wall -o client ClientRunner.c libnetfiles.c

server:
	gcc -Wall -pthread -o server netfileserver.c

clean:
	rm -f client server
