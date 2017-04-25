all: client server

client:
	gcc -Wall -o client ClientRunner.c libnetfiles.c

server:
	gcc -Wall -pthread -o server netfileserver.c

test: server
	gcc -Wall -o myCli myCliTester.c libnetfiles.c

ctest:
		rm -f myCli server

clean:
	rm -f client server
