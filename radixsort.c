/*
 *                   RADIXSORT.C
 *
 * "radixsort.c" is an initialization file for the openCL
 * implementation of the Radix Sort algorithm.
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

//Kernel defines
#ifndef _RS_FILLFUN_
#define _RS_FILLFUN_ generateArray()
/*Define an array filling function for testing (random)*/
int *generateArray(void)
{
    int i, *array = (int*)malloc(sizeof(int) * ARRLEN);
    for(i=0; i<ARRLEN; i++){
        array[i] = rand() % 10000;
    }   
    return array;
}
#endif

//Function to determine a file size (from the current cursor pos.)
int filesize(FILE *fp) {
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, prev, SEEK_SET); //return cursor to prev. pos.
    return size;
}


int main() {

    //----------------------
    // Initialize host data
    //----------------------
    int *array = NULL; //Input array
    int *output = NULL; //Output array
    
    //Data array size
    size_t array_dataSize = sizeof(int)*ARRLEN;

    //Allocate space for the arrays
    array = (int*)malloc(array_dataSize);
    output = (int*)malloc(array_dataSize);

    //Initialize array data
    array = _RS_FILLFUN_;

    cl_int errNum;

    //----------------
    // Import kernels
    //----------------
    FILE *fp;
    const char file_name[] = KERNELS_FILENAME; /*TODO: AGREGAR AL .h*/
    size_t file_sourceSize;
    char *file_sourceStr;

    fp = fopen(file_name, "r");
    if(!fp) {
        printf("Error opening the kernels file: [%s]\n", file_name);
        exit(1);
    }
    file_sourceSize = filesize(fp); //TODO: Ojo con este casteo
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
    cl_mem output_buffer;

    //Create input buff
    array_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, array_dataSize, NULL, &errNum);
    //Create output buff
    output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, array_dataSize, NULL, &errNum);

    //----------------------
    // Enqueue device write (host -> device buffer)
    //----------------------
    
    errNum = clEnqueueWriteBuffer(commandQueue, array_buffer, CL_FALSE /*TODO*/, 0, array_dataSize, array, 0, NULL, NULL);

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
    errNum = clBuildProgram(program, 1, devices, "-cl-std=CL1.1", NULL, NULL);
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
    cl_kernel *kernels = (cl_kernel*)malloc(sizeof(cl_kernel)*KERNELS_NUMBER); //More than one kernel in kernel file
    
    //Take all kernels from file
    errNum = clCreateKernelsInProgram(program, KERNELS_NUMBER, kernels, NULL);
    if(!errNum == CL_SUCCESS){
        printf("Error obtaining kernels. Using \"clCreateKernelsInProgram\"\n");
        exit(1);
    }
    
    //Separate kernels (manually)
    cl_kernel count, scan, reorder;
    char kernelName[MAX_KERNEL_NAME];
    int i;
    for(i=0; i < KERNELS_NUMBER; i++){
        errNum = clGetKernelInfo(kernels[i], CL_KERNEL_FUNCTION_NAME, sizeof(kernelName), kernelName, NULL);
        if(errNum != CL_SUCCESS){
            printf("Error renaming kernels. Using \"clGetKernelInfo\"\n");
            exit(1);
        }
        if(strcmp(kernelName, "count") == 0) 
            count = kernels[i];
        if(strcmp(kernelName, "scan") == 0) 
            scan = kernels[i];
//        if(strcmp(kernelName, "reorder") == 0) 
//            reorder = kernels[i];
    }   
    
    if((count == NULL) || (scan == NULL) /*|| (reorder == NULL)*/){
        printf("ERROR");
        exit(1);
    }

    //----------------------
    // Set kernel arguments
    //----------------------

    //Count arguments
    int pass = 0;
    int arrlen = ARRLEN;
    errNum = clSetKernelArg(count, 0, sizeof(cl_mem), &array_buffer);    // Input array /*TODO: Change with pass*/
    errNum |= clSetKernelArg(count, 1, sizeof(cl_mem), &output_buffer);  // Output array
    errNum |= clSetKernelArg(count, 2, sizeof(int), &pass);              // Pass number /*TODO: Change with pass*/
    errNum |= clSetKernelArg(count, 3, sizeof(int), &arrlen);            // Number of elements in array
    errNum |= clSetKernelArg(count, 4, sizeof(uint)*BUCK*ITEMS

    //Scan arguments
    errNum = clSetKernelArg(scan, 0, sizeof(cl_mem), &array_buffer);     // Input array
    errNum |= clSetKernelArg(scan, 1, sizeof(cl_mem), &output_buffer);   // Output array
    errNum |= clSetKernelArg(scan, 2, sizeof(int), &arrlen);             // Number of elements in array

    //Reorder arguments
    //TODO


    //---------------------------------
    // Configure work-item structuring
    //---------------------------------
    size_t globalWorkSize[1];
    
    globalWorkSize[0] = ARRLEN;

    //-------------------------------
    // Enqueue kernels for execution
    //-------------------------------
    
    errNum = clEnqueueNDRangeKernel(commandQueue, count, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    errNum = clEnqueueNDRangeKernel(commandQueue, scan, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);

    //-------------------
    // Enqueue host read (device buffer -> host)
    //-------------------
    
    clEnqueueReadBuffer(commandQueue, output_buffer, CL_TRUE, 0, array_dataSize, output, 0, NULL, NULL);
    
    //Verify output with normal function
    //bool result = true;
    //TODO: VERIFICADOR DE ORDEN


    //******************+TESTING+**********************
    
    for(i=0; i<ARRLEN; i++) {
        printf("[%d]", array[i]);
    }
    printf("\n\n");
    for(i=0; i<ARRLEN; i++) {
        printf("[%d]", output[i]);
    }
    printf("\n\n");

    //******************-TESTING-**********************

    //----------------
    // Free resources
    //----------------
    //openCL
    clReleaseKernel(count);
    clReleaseKernel(scan);
//    clReleaseKernel(reorder);
    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    clReleaseMemObject(array_buffer); clReleaseMemObject(output_buffer);
    clReleaseContext(context);
    //Host
    free(array); free(output);
    free(platforms);
    free(devices);
    
    return 0;
}
