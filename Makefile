CC=gcc
CFLAGS=-g -Wall -std=c99 -I/usr/local/cuda/include/ -L/usr/local/cuda/lib64/
CFLAGS_COMP= -g -Wall -Wno-comment
LIBS=-lOpenCL
hello_world: radixsort.c radixsort.h radixsort.cl
	$(CC) radixsort.c -o radixmain $(CFLAGS) $(CFLAGS_COMP) $(LIBS)
