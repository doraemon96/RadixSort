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

#include <time.h>
#include <inttypes.h>

//OpenCL includes
#include <CL/opencl.h>

//Kernel includes
#include "radixsort.h"
#include "checkorder.c"

int *radixsort(int *array, int size);
int cmpfunc (const void * a, const void * b)
{
    return ( *(int*)a - *(int*)b );
}

int isPowerOfTwo(int x)
{
    return ((x != 0) && ((x & (x - 1)) == 0)) ? 1 : 0;
}


int main(void)
{

    int i, *array = malloc(sizeof(int) * ARRLEN);
    #ifdef DEBUG
    int constarr[8] = {120,223,102,300,335,160,253,111};
    for(i=0; i<ARRLEN; i++)
        array[i] = constarr[i];
    #else
    /*Define an array filling function for testing (random)*/
    for(i=0; i<ARRLEN; i++){
    array[i] = rand() % ARRLEN;
    }   
    #endif

    int *sorted;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    //Call radixsort
    sorted = radixsort(array, ARRLEN);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Radixsort of %d numbers took %" PRIu64 " microseconds\n", ARRLEN, delta);

/*
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    //Call quicksort
    qsort(array, ARRLEN, sizeof(int), cmpfunc);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    delta = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Quicksort of %d numbers took %" PRIu64 " microseconds\n", ARRLEN, delta);
*/

    //Check if sorted with parallel check!
    checkorder(sorted,ARRLEN);

/*
    //Check against qsort
    for(i=0; i<ARRLEN; i++){
        if(array[i] != sorted[i]){
            printf("Differs!\n");
            exit(1);
        }
    }
*/
 
#ifdef PRINT
#endif
    //Deactivate qsort before printing
    printf("Arreglo Original:\n");
    for(i=0; i<ARRLEN; i++) {
        printf("[%d]", array[i]);
    }
    printf("\n\n");  
    printf("Arreglo Ordenado:\n");
    for(i=0; i<ARRLEN; i++) {
        printf("[%d]", sorted[i]);
    }
    printf("\nCantidad de elementos en sorted: %d\n", i);
    printf("\n\n");
  
    return 0;
}


//Function to determine a file size (from the current cursor pos.)
int filesize(FILE *fp) {
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, prev, SEEK_SET); //return cursor to prev. pos.
    return size;
}


//**********************************************
// radixsort
//
//   Takes an int array pointer an its size and
//   returns a sorted array
//**********************************************
int *radixsort(int *array, int size) {

    //Disable caching for nvidia, helps with .h files included in kernel
    setenv("CUDA_CACHE_DISABLE", "1", 1);

    //----------------------
    // Initialize host data
    //----------------------
    int *output = NULL; //Output array
    size_t array_dataSize = sizeof(int)*size;
    output = (int*)malloc(array_dataSize);

    //Allocate space for the arrays

    cl_int errNum;

    //----------------
    // Import kernels
    //----------------
    FILE *fp;
    const char file_name[] = KERNELS_FILENAME;
    size_t file_sourceSize;
    char *file_sourceStr;

    fp = fopen(file_name, "r");
    if(!fp) {
        printf("Error opening the kernels file: [%s]\n", file_name);
        exit(1);
    }
    file_sourceSize = filesize(fp);
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
    errNum = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
    //Alloc device spaces
    devices = (cl_device_id*)malloc(numDevices*sizeof(cl_device_id));
    //Fill with device info
    errNum = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, numDevices, devices, NULL);


#ifdef PRINT
    //Print local memory sizes
    cl_ulong local_mem_size;
    clGetDeviceInfo(devices[0], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem_size, 0);
    int local_mem = local_mem_size;
    printf("\n\nLocal mem size: %d\n\n", local_mem);
#endif


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
    cl_mem histo_buffer;
    cl_mem scan_buffer;
    cl_mem blocksum_buffer;
    cl_mem output_buffer;

    //Create input buff
    array_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, array_dataSize, NULL, &errNum);
    //Create histo buff
    histo_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int) * BUCK * N_GROUPS * WG_SIZE, NULL, &errNum);
    //Create scan buff
    scan_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int) * BUCK * N_GROUPS * WG_SIZE, NULL, &errNum);
    //Create blocksum buff
    blocksum_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int) * N_GROUPS, NULL, &errNum);
    //Create output buff
    output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, array_dataSize, NULL, &errNum);


    //----------------------
    // Enqueue device write (host -> device buffer)
    //----------------------
    
    errNum = clEnqueueWriteBuffer(commandQueue, array_buffer, CL_FALSE, 0, array_dataSize, array, 0, NULL, NULL);
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

    cl_kernel count, scan, blocksum, coalesce, reorder;
    count = clCreateKernel(program, "count", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating count kernel\n");
        exit(1);
    }
    scan = clCreateKernel(program, "scan", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating scan kernel\n");
        exit(1);
    }
    blocksum = clCreateKernel(program, "scan", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating blocksum kernel\n");
        exit(1);
    }
    coalesce = clCreateKernel(program, "coalesce", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating coalesce kernel\n");
        exit(1);
    }
    reorder = clCreateKernel(program, "reorder", &errNum);
    if(!errNum == CL_SUCCESS){
        printf("Error creating reorder kernel\n");
        exit(1);
    }

    //-------------------------------
    // Set kernels constant arguments
    //-------------------------------

    //Count fixed args
    size_t CountGlobalWorkSize = N_GROUPS * WG_SIZE;
    size_t CountLocalWorkSize = WG_SIZE;
    errNum = clSetKernelArg(count, 2, sizeof(int)*BUCK*WG_SIZE, NULL);  // Local Histogram
    errNum |= clSetKernelArg(count, 4, sizeof(int), &size);           // Number of elements in array

    //Scan fixed args
    size_t ScanGlobalWorkSize = (BUCK * N_GROUPS * WG_SIZE) / 2;
    size_t ScanLocalWorkSize = ScanGlobalWorkSize / N_GROUPS;
    errNum = clSetKernelArg(scan, 1, sizeof(cl_mem), &scan_buffer);       // Output array
    errNum |= clSetKernelArg(scan, 2, sizeof(int)*BUCK*WG_SIZE, NULL);    // Local Scan
    errNum |= clSetKernelArg(scan, 3, sizeof(cl_mem), &blocksum_buffer);  // Block Sum

    //Blocksum fixed args
    size_t BlocksumGlobalWorkSize = N_GROUPS / 2;
    size_t BlocksumLocalWorkSize =  N_GROUPS / 2;
    void* ptr = NULL;
    errNum = clSetKernelArg(blocksum, 0, sizeof(cl_mem), &blocksum_buffer);   // Input array
    errNum |= clSetKernelArg(blocksum, 1, sizeof(cl_mem), &blocksum_buffer);  // Output array
    errNum |= clSetKernelArg(blocksum, 2, sizeof(int)*N_GROUPS, NULL);        // Local Scan
    errNum |= clSetKernelArg(blocksum, 3, sizeof(cl_mem), ptr);               // Block Sum (null)

    //Coalesce fixed args
    size_t CoalesceGlobalWorkSize = (BUCK * N_GROUPS * WG_SIZE) / 2;
    size_t CoalesceLocalWorkSize = CoalesceGlobalWorkSize / N_GROUPS;
    errNum = clSetKernelArg(coalesce, 0, sizeof(cl_mem), &scan_buffer);      // Scan array
    errNum |= clSetKernelArg(coalesce, 1, sizeof(cl_mem), &blocksum_buffer);  // Block reductions

    //Reorder fixed args
    size_t ReorderGlobalWorkSize = N_GROUPS * WG_SIZE;
    size_t ReorderLocalWorkSize = WG_SIZE;
    errNum = clSetKernelArg(reorder, 1, sizeof(cl_mem), &scan_buffer);    //Prefix Sum array
    errNum |= clSetKernelArg(reorder, 4, sizeof(int), &size);            // Number of elements in array
    errNum |= clSetKernelArg(reorder, 5, sizeof(int)*BUCK*WG_SIZE, NULL);  // Local Histogram


    //-------------------------------
    // Enqueue kernels for execution
    //-------------------------------

    int pass;
#ifdef DEBUG //Do only DEBUG passes
    for(pass = 0; pass < DEBUG; pass++){
#else        //Operate normaly
    for(pass = 0; pass < BITS/RADIX; pass++){
#endif
#ifdef PRINT
        printf("Currently on pass:[%d]\n",pass);
#endif

        //Count arguments
        errNum = clSetKernelArg(count, 0, sizeof(cl_mem), &array_buffer);   // Input array
        errNum |= clSetKernelArg(count, 1, sizeof(cl_mem), &histo_buffer);  // Output array
        errNum |= clSetKernelArg(count, 3, sizeof(int), &pass);             // Pass number
        errNum = clEnqueueNDRangeKernel(commandQueue, count, 1, NULL, &CountGlobalWorkSize, &CountLocalWorkSize, 0, NULL, NULL);
        if(!errNum == CL_SUCCESS){
            printf("Count kernel terminated abruptly\n");
            switch(errNum) {
                case CL_INVALID_KERNEL_ARGS:
                    printf("Invalid kernel args\n");
                    break;
                case CL_OUT_OF_RESOURCES:
                    printf("Out of resources\n");
                    break;
                case CL_MEM_OBJECT_ALLOCATION_FAILURE:
                    printf("Memobject allocation failure\n");
                    break;
                default:
                    printf("Unspecified case\n");
            }
 
            exit(1);
        }
        clFinish(commandQueue);
    #ifdef DEBUG
        int* countput;
        countput = (int*)malloc(sizeof(int)*BUCK*WG_SIZE*N_GROUPS);
        errNum = clEnqueueReadBuffer(commandQueue, histo_buffer, CL_TRUE, 0, sizeof(int)*BUCK*WG_SIZE*N_GROUPS, countput, 0, NULL, NULL);
        clFinish(commandQueue);
    #endif


        //Scan arguments
        errNum = clSetKernelArg(scan, 0, sizeof(cl_mem), &histo_buffer);      // Input array
        errNum = clEnqueueNDRangeKernel(commandQueue, scan, 1, NULL, &ScanGlobalWorkSize, &ScanLocalWorkSize, 0, NULL, NULL);
        if(!errNum == CL_SUCCESS){
            printf("Scan kernel terminated abruptly\n");
            switch(errNum) {
                case CL_INVALID_KERNEL_ARGS:
                    printf("Invalid kernel args\n");
                    break;
                default:
                    printf("Unspecified case\n");
            }
                
            exit(1);
        }
        clFinish(commandQueue);
    #ifdef DEBUG
        int* scanput;
        scanput = (int*)malloc(sizeof(int)*BUCK*WG_SIZE*N_GROUPS);
        errNum = clEnqueueReadBuffer(commandQueue, histo_buffer, CL_TRUE, 0, sizeof(int)*BUCK*WG_SIZE*N_GROUPS, scanput, 0, NULL, NULL);
        int* oblockput;
        oblockput = (int*)malloc(sizeof(int)*N_GROUPS);
        errNum = clEnqueueReadBuffer(commandQueue, blocksum_buffer, CL_TRUE, 0, sizeof(int)*N_GROUPS, oblockput, 0, NULL, NULL);
        clFinish(commandQueue);
    #endif


        //Block Sum arguments
        errNum = clEnqueueNDRangeKernel(commandQueue, blocksum, 1, NULL, &BlocksumGlobalWorkSize, &BlocksumLocalWorkSize, 0, NULL, NULL);
        if(!errNum == CL_SUCCESS){
            printf("Block Sum kernel terminated abruptly\n");
            switch(errNum){
                case CL_INVALID_KERNEL_ARGS:
                    printf("Invalid kernel args\n");
                    break;
                default:
                    printf("Unspecified case\n");
            }
            exit(1);
        }
        clFinish(commandQueue);
    #ifdef DEBUG
        int* blockput;
        blockput = (int*)malloc(sizeof(int)*N_GROUPS);
        errNum = clEnqueueReadBuffer(commandQueue, blocksum_buffer, CL_TRUE, 0, sizeof(int)*N_GROUPS, blockput, 0, NULL, NULL);
        clFinish(commandQueue);
    #endif


        //Coalesce arguments
        errNum = clEnqueueNDRangeKernel(commandQueue, coalesce, 1, NULL, &CoalesceGlobalWorkSize, &CoalesceLocalWorkSize, 0, NULL, NULL);
        if(!errNum == CL_SUCCESS){
            printf("Coalesce kernel terminated abruptly\n");
            exit(1);
        }
        clFinish(commandQueue);
    #ifdef DEBUG
        int* coalput;
        coalput = (int*)malloc(sizeof(int)*BUCK*WG_SIZE*N_GROUPS);
        errNum = clEnqueueReadBuffer(commandQueue, scan_buffer, CL_TRUE, 0, sizeof(int)*BUCK*WG_SIZE*N_GROUPS, coalput, 0, NULL, NULL);
        clFinish(commandQueue);
    #endif


        //Reorder arguments
        errNum = clSetKernelArg(reorder, 0, sizeof(cl_mem), &array_buffer);       // Input array
        errNum |= clSetKernelArg(reorder, 2, sizeof(cl_mem), &output_buffer);
        errNum |= clSetKernelArg(reorder, 3, sizeof(int), &pass);                 // Pass number
        errNum = clEnqueueNDRangeKernel(commandQueue, reorder, 1, NULL, &ReorderGlobalWorkSize, &ReorderLocalWorkSize, 0, NULL, NULL);
        if(!errNum == CL_SUCCESS){
            printf("Reorder kernel terminated abruptly\n");
            switch(errNum) {
                case CL_INVALID_KERNEL_ARGS:
                    printf("Invalid kernel args\n");
                    break;
                default:
                    printf("Unspecified case\n");
            }
                
            exit(1);
        }
        clFinish(commandQueue);


        //Swap current array with newest array
        cl_mem tmp = array_buffer;
        array_buffer = output_buffer;
        output_buffer = tmp;

    }

    //-------------------
    // Enqueue host read (device buffer -> host)
    //-------------------
    
    errNum = clEnqueueReadBuffer(commandQueue, output_buffer, CL_TRUE, 0, array_dataSize, output, 0, NULL, NULL);
    clFinish(commandQueue);
    
    
#ifdef DEBUG
    int k;
    printf("Arreglo Original:\n");
    for(k=0; k<ARRLEN; k++) {
        printf("[%d]", array[k]);
    }
    printf("\n\n");
    printf("Resultado Count:");
    for(k=0; k<BUCK*WG_SIZE*N_GROUPS; k++) {
        printf("[%d]", countput[k]);
    }
    printf("\n\n");
    printf("Resultado Scan:");
    for(k=0; k<BUCK*WG_SIZE*N_GROUPS; k++) {
        printf("[%d]", scanput[k]);
    }
    printf("\n\n");
    printf("Resultado Block Array:");
    for(k=0; k<N_GROUPS; k++) {
        printf("[%d]", oblockput[k]);
    }
    printf("\n\n");
    printf("Resultado Block Sum:");
    for(k=0; k<N_GROUPS; k++) {
        printf("[%d]", blockput[k]);
    }
    printf("\n\n");
    printf("Resultado Coalesce:");
    for(k=0; k<BUCK*WG_SIZE*N_GROUPS; k++) {
        printf("[%d]", coalput[k]);
    }
    printf("\n\n");
    printf("Resultado Ordenado:");
    for(k=0; k<ARRLEN; k++) {
        printf("[%d]", output[k]);
    }
    printf("\n\n");
#endif

    //----------------
    // Free resources
    //----------------
    //openCL
    clReleaseKernel(count);
    clReleaseKernel(scan);
    clReleaseKernel(blocksum);
    clReleaseKernel(coalesce);
    clReleaseKernel(reorder);

    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    
    clReleaseMemObject(array_buffer);
    clReleaseMemObject(histo_buffer);
    clReleaseMemObject(scan_buffer);
    clReleaseMemObject(blocksum_buffer);
    clReleaseMemObject(output_buffer);

    clReleaseContext(context);
    //Host
#ifdef DEBUG
    free(countput);
    free(scanput);
    free(coalput);
    free(oblockput);
    free(blockput);   
#endif
    free(platforms);
    free(devices);
    
    //---------------------
    // Return sorted array
    //---------------------
    return output;
}
