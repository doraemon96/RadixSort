CC=g++
OPENCL_LIB_DIR=/opt/AMDAPP/lib/x86_64/
OPENCL_INCLUDE_DIR=/opt/AMDAPP/include/
CFLAGS=-g -Wall  -I$(OPENCL_INCLUDE_DIR) -L$(OPENCL_LIB_DIR)
LIBS=-lOpenCL
hello_world:
	$(CC) radixsort.c -o hello_world $(CFLAGS) $(LIBS)
