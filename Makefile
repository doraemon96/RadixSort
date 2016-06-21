CC=gcc
CFLAGS=-g -Wall -std=c99 -I/usr/local/cuda/include/ 
CFLAGS_COMP= -g -Wall -Wno-comment
LIBS=-lOpenCL
hello_world:
	$(CC) radixsort.c -o radixmain $(CFLAGS) $(CFLAGS_COMP) $(LIBS)
