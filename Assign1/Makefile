all: server deliver
server : server.c
	gcc server.c -o server
deliver : deliver.c
	gcc deliver.c -o deliver

.PHONY: clean
clean:
	rm -f all server deliver
