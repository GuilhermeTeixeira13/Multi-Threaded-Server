CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder, proofofwork

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

proofofwork:	proofofwork.c
	$(CC) $(CFLAGS) -o proofofwork proofofwork.c

clean:
	rm -f adder *~
