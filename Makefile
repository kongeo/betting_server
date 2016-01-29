#!/bin/make

all: clean betting_server doxygen #betting_client

betting_server:
	gcc -g -o betting_server betting_server.c main.c -lpthread

doxygen:
	doxygen betting.dox

betting_client:
	gcc -g -o betting_client betting_client.c

clean:
	rm -rf betting_server html/ #betting_client
