/*
 *                   CHECKORDER.C
 *
 * "checkorder.c" is an initialization file for the openCL
 * implementation of a parallel order checking algorithm.
 *
 * 2016 Project for the "Facultad de Ciencias Exactas, Ingenieria
 * y Agrimensura" (FCEIA), Rosario, Santa Fe, Argentina.
 *
 * Implementation by Paoloni Gianfranco and Soncini Nicolas.
 */

//System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//OpenCL includes
#include <CL/opencl.h>

//Kernel includes
#include "radixsort.h"


//Function to determine a file size (from the current cursor pos.)
int filesizer(FILE *fp) {
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, prev, SEEK_SET); //return cursor to prev. pos.
    return size;
}


void checkorder(int *array, int size) {

    //Disable caching for nvidia, helps with .h files included in kernel
    setenv("CUDA_CACHE_DISABLE", "1", 1);

    //Data array size
    size_t array_dataSize = sizeof(int)*size;

    cl_int errNum;

    //----------------
    // Import kernels
    //----------------
    FILE *fp;
    const char file_name[] = "checkorder.cl";
    size_t file_sourceSize;
    char *file_sourceStr;

    fp = fopen(file_name, "r");
    if(!fp) {
        printf("Error opening the kernels file: [%s]\n", file_name);
        exit(1);
    }
    file_sourceSize = filesizer(fp);
    file_sourceStr = (char*)malloc(file_sourceSize);
    if(file_sourceSize != fread(file_sourceStr, 1, file_sourceSize, fp)) {
        printf("Error reading the kernels file: [%s]\n", file_name);
        exit(1);
    }
    fclose(fp);


    //----------------------
    // Obtain platform info
    //----------------------
    cl_uint numPlatforms = 0;
    cl_platform_id *platforms = NULL;

    //Obtain platform number (mockcall)
    errNum = clGetPlatformIDs(0, NULL, &numPlatforms);
    //Alloc space per platform
    platforms = (cl_platform_id*)malloc(numPlatforms*sizeof(cl_platform_id));
    //Fill with platform info
    errNum = clGetPlatformIDs(numPlatforms, platforms, NULL);

    //--------------------
    // Obtain device info
    //--------------------
    cl_uint numDevices = 0;
    cl_device_id *devices = NULL;

    //Obtain device number(mockcall)
    errNum = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL /*TODO GPU*/, 0, NULL, &numDevices);
    //Alloc device spaces
    devices = (cl_device_id*)malloc(numDevices*sizeof(cl_device_id));
    //Fill with device info
    errNum = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL /*TODO ^^^*/, numDevices, devices, NULL);

    //----------------
    // Create context
    //----------------
    cl_context context = NULL;

    //Create device bound context
    context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &errNum);

    //----------------------
    // Create command queue
    //----------------------
    cl_command_queue commandQueue;
    
    commandQueue = clCreateCommandQueue(context, devices[0], 0, &errNum);

    //----------------
    // Create buffers
    //----------------
    cl_mem array_buffer;
    cl_mem compare_buffer;
    cl_mem reduce_return;

    //Create compare buff
    array_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, array_dataSize, NULL, &errNum);
    compare_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, array_dataSize, NULL, &errNum);
    reduce_return = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, &errNum);

    //----------------------
    // Enqueue device write (host -> device buffer)
    //----------------------
    
    errNum = clEnqueueWriteBuffer(commandQueue, array_buffer, CL_FALSE /*TODO*/, 0, array_dataSize, array, 0, NULL, NULL);
    if(!errNum == CL_SUCCESS){
        printf("Array buffer write terminated abruptly\n");
        exit(1);
    }
    clFinish(commandQueue);

    //----------------------------
    // Create and compile program
    //----------------------------
    cl_program program = NULL;
    
    //Create program from file
    program = clCreateProgramWithSource(context, 1, (const char**)&file_sourceStr, (const size_t*)&file_sourceSize, &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error obtaining program from source. Using \"clCreateProgramWithSource\"\n");
        exit(1);
    }

    //Compile for openCL 1.1
    errNum = clBuildProgram(program, 1, devices, "-I. -cl-std=CL1.1", NULL, NULL);
    if(!errNum == CL_SUCCESS){
        printf("Error building program. Using \"clBuildProgram\"\n");
        if(errNum == CL_BUILD_PROGRAM_FAILURE){
            printf("Failure to build the program executable\n");
            size_t log_size;
            clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
            char *log = (char *) malloc(log_size);
            clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            printf("*** BUILD INFO LOG *** \n%s\n", log);
            free(log);
        }
        exit(1);
    }

    //----------------
    // Create kernels
    //----------------

    cl_kernel parallelcmp, reduce;
    parallelcmp = clCreateKernel(program, "parallelcmp", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating parallelcmp kernel\n");
        exit(1);
    }
    reduce = clCreateKernel(program, "reduce", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating reduce kernel\n");
        exit(1);
    }

    //-------------------------------
    // Set kernels constant arguments
    //-------------------------------
    int arrlen = size;

    //Order-check kernels args
    size_t globalWorkSizeCmp = N_GROUPS * WG_SIZE;
    size_t localWorkSizeCmp = WG_SIZE;
    errNum = clSetKernelArg(parallelcmp, 0, sizeof(cl_mem), &array_buffer);
    errNum |= clSetKernelArg(parallelcmp, 1, sizeof(cl_mem), &compare_buffer);
    errNum |= clSetKernelArg(parallelcmp, 2, sizeof(int), &arrlen);

    size_t globalWorkSizeReduce = (BUCK * N_GROUPS * WG_SIZE) / 2;
    size_t localWorkSizeReduce = globalWorkSizeReduce / N_GROUPS;
    errNum = clSetKernelArg(reduce, 0, sizeof(cl_mem), &compare_buffer);
    errNum |= clSetKernelArg(reduce, 1, sizeof(cl_mem), &reduce_return);
    errNum |= clSetKernelArg(reduce, 2, sizeof(int), &arrlen);


    //-------------------------------
    // Enqueue kernels for execution
    //-------------------------------

    //Parallel Cmp
    errNum = clEnqueueNDRangeKernel(commandQueue, parallelcmp, 1, NULL, &globalWorkSizeCmp, &localWorkSizeCmp, 0, NULL, NULL);
    if(!errNum == CL_SUCCESS){
        printf("Parallel cmp kernel terminated abruptly\nERROR code: %d\n", errNum);
        exit(1);
    }
    clFinish(commandQueue);

    //Reduce
    errNum = clEnqueueNDRangeKernel(commandQueue, reduce, 1, NULL, &globalWorkSizeReduce, &localWorkSizeReduce, 0, NULL, NULL);
    if(!errNum == CL_SUCCESS){
        printf("Reduce kernel terminated abruptly\nERROR code: %d\n", errNum);
        exit(1);
    }
    clFinish(commandQueue);

    int* cmpput = NULL;
    cmpput = (int*)malloc(sizeof(int));

    errNum = clEnqueueReadBuffer(commandQueue, reduce_return, CL_TRUE, 0, sizeof(int), cmpput, 0, NULL, NULL);
    clFinish(commandQueue);

    
    //-------------------------------
    // Print result from compares
    //-------------------------------

    if(cmpput[0])
        printf("Arreglo desordenado. Cantidad de elementos fuera de lugar: %d\n", cmpput[0]);
    else
        printf("Arreglo ordenado.");
    printf("\n\n");


    //----------------
    // Free resources
    //----------------
    //OpenCL
    clReleaseKernel(parallelcmp);
    clReleaseKernel(reduce);

    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    
    clReleaseMemObject(compare_buffer);
    clReleaseMemObject(reduce_return);

    clReleaseContext(context);
    //Host
    free(platforms);
    free(devices);
}
