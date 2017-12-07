CC = gcc
CFLAGS = -g -Wall -I/usr/local/cuda/include/ -L/usr/local/cuda/lib64/
CFLAGS_COMP = -g -Wall -Wno-comment
LIBS = -lOpenCL

DEPS = radixsort.h
OBJS = radixsort.o

labdcc: $(OBJS)
	$(CC) $(OBJS) -o radixmain $(CFLAGS) $(CFLAGS_COMP) $(LIBS)

%.o:%.c $(DEPS)
	$(CC) -c -g -o $@ $< $(CFLAGS) $(CFLAGS_COMP)
