all: client server

client:
	gcc -Wall -o client ClientRunner.c

server:
	gcc -Wall -o server netfileserver.c

clean:
	rm -f client server
