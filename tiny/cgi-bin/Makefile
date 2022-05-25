CC = gcc

#Release
CFLAGS = -O2 -Wall -I ..
#Debug
#CFLAGS = -g -Wall -I ..

all: adder proofofwork
OBJS = sha1.o sha256.o sha384.o sha512.o md5.o rmd160.o

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c
proofofwork: proofofwork.c $(OBJS)
	$(CC) $(CFLAGS) -o proofofwork proofofwork.c sha1.o

clean:
	rm -f adder *~
