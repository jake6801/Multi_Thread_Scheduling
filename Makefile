all: mts

mts: mts.c priority_queue.c
	gcc mts.c priority_queue.c -pthread -o mts 

clean: 
	rm -f mts