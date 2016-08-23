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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "radixsort.h"
#include <CL/opencl.h>

#ifndef _RS_FILLFUN_
#define _RS_FILLFUN_ generateArray()
/*Definimos una funcion de testeo (random)*/
int *generateArray(void)
{
    int i, *array = (int*)malloc(sizeof(int) * ARRLEN);
    for(i=0; i<ARRLEN; i++){
        array[i] = rand() % 10000;
    }
    return array;
}
#endif


int main(void){

    cl_int errNum;

    FILE *fp;
    const char fileName[] = "./radixsort.cl";
    size_t source_size;
    char *source_str;

    fp = fopen(fileName, "r");
    if(!fp){
        printf("Error al cargar \"%s\"\n", fileName);
        return(1);
    }
    source_str = (char *)malloc(MAX_SOURCE_SIZE);               /*Podria hacer algo mas descriptivo que un DEFINE*/
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);    /*Lee a source_str y deja en source_size la cantidad de bytes(o chars?) leidos*/
    fclose(fp);

    /*Tomar info de plataformas y devices*/
    cl_platform_id platform_id;
    cl_uint num_platforms;
    cl_device_id device_id;
    cl_uint num_devices;

    errNum = clGetPlatformIDs(1, &platform_id, &num_platforms);
    errNum |= clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &num_devices);	
    if(errNum != CL_SUCCESS){
        printf("Error al obtener ID de Plataform/Device.\n");
        exit(1);
    }
    size_t dev_name_size;
    errNum = clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &dev_name_size);
    char *dev_name = malloc(dev_name_size);
    errNum = clGetDeviceInfo(device_id, CL_DEVICE_NAME, dev_name_size, dev_name, NULL);
    printf("Device info: %s\n", dev_name);

    /*Crear contexto y cola de comandos*/
    cl_context context;
    cl_command_queue command_queue;
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &errNum);
    command_queue = clCreateCommandQueue(context, device_id, 0, &errNum);

    /*Crear programa de kernels desde el source */
    cl_program program = NULL;
    program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t *)&source_size, &errNum);
    
    /*Compilar programa*/
    errNum = clBuildProgram(program, 1, &device_id, "-cl-std=CL1.1", NULL, NULL);
    if(errNum != CL_SUCCESS){
        printf("Error al crear el programa con \"clBuildProgram\": ");
        switch(errNum){
            case CL_INVALID_PROGRAM:        printf("program is an invalid program object.\n"); break;
            case CL_INVALID_DEVICE:         printf("device_id is not associated with program.\n"); break;
            case CL_INVALID_BUILD_OPTIONS:  printf("build options are invalid.\n"); break;
            case CL_COMPILER_NOT_AVAILABLE: printf("a compiler is not available for \"clCreateProgramWithSource\".\n"); break;
            case CL_BUILD_PROGRAM_FAILURE:  
            {
                printf("failure to build the program executable.\n"); 
                size_t log_size;
                clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
                char *log = (char *) malloc(log_size);
                clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
		printf("*** BUILD INFO LOG *** \n%s\n", log);
		free(log);
                break;
            }
            case CL_OUT_OF_RESOURCES:       printf("error allocating resources by the OpenCL device implementation.\n"); break;
            case CL_INVALID_OPERATION:      printf("kernel objects are already attached to program.\n"); break;
            default: printf("Unmatched error.\n");
        }
        exit(1);
    }
    
    /*Crear kernel*/
    cl_kernel *kernels = (cl_kernel*)malloc(N_KERNELS * sizeof(cl_kernel));
    errNum = clCreateKernelsInProgram(program, N_KERNELS, kernels, NULL); //errNum es uint y esto devuelve int, posible error de compilación?
    if(errNum != CL_SUCCESS){
        printf("Error al crear Kernels con \"clCreateKernels\".\n");
        exit(1);
    }

    /*Separar kernels (con nombre)*/
    cl_kernel count, scan, reorder;
    char kernelName[MAX_KERNEL_NAME];
    int i;
    for(i=0; i < N_KERNELS; i++){
        errNum = clGetKernelInfo(kernels[i], CL_KERNEL_FUNCTION_NAME, sizeof(kernelName), kernelName, NULL);
        if(errNum != CL_SUCCESS){
            printf("Error al renombrar kernels. Usando \"clGetKernelInfo\".\n");
            exit(1);
        }
        if(strcmp(kernelName, "count") == 0){
            count = kernels[i];
        }
        if(strcmp(kernelName, "naivedbParallelScan") == 0){
            scan = kernels[i];
        }
        if(strcmp(kernelName, "reorder") == 0){
            reorder = kernels[i];
        }
    }
    
    if((count == NULL) || (scan == NULL) || (reorder == NULL)){
        printf("ERROR");
	exit(1);
    }

    /*Buffers*/
    /*Create initial buffer (array to be sorted)*/
    int *array = _RS_FILLFUN_;
    int ret_count[ARRLEN] = {69};

    cl_uint max_cunits;
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), (void*)&max_cunits, NULL);
    if((max_cunits * (cl_uint)WORKGROUP_SZ) < (cl_uint)ARRLEN){	
        printf("Not defined yet\nWorkItemNumber: %ud\n", max_cunits * (cl_uint)WORKGROUP_SZ);
        exit(1);
    }

    printf("WorkItemNumber: %d\n", max_cunits * (cl_uint)WORKGROUP_SZ);
    
    size_t sz = sizeof(int)*ARRLEN;
    cl_mem input = clCreateBuffer(context, CL_MEM_READ_WRITE, sz, &array, &errNum);
    cl_mem output = clCreateBuffer(context, CL_MEM_READ_WRITE, sz, NULL, &errNum);
    cl_mem count_ret = clCreateBuffer(context, CL_MEM_READ_WRITE, sz, &ret_count, &errNum);
    cl_mem scan_ret = clCreateBuffer(context, CL_MEM_READ_WRITE, sz, NULL, &errNum);
 

    /*Setear parametros a kernels*/
    /*Count*/
    /*TEST*/ int bit = 0;
    errNum  = clSetKernelArg(count, 0, sizeof(cl_mem), (void*)&input);
    errNum |= clSetKernelArg(count, 1, sizeof(cl_mem), (void*)&count_ret);
    errNum |= clSetKernelArg(count, 2, sizeof(int), (void*)&bit);

    /*Scan*/
//    errNum  = clSetKernelArg(scan, 0, /*TODO:argsize*/, /*TODO:argval int *input*/);
//    errNum |= clSetKernelArg(scan, 1, /*TODO:argsize*/, /*TODO:argval int *output*/);
//    errNum |= clSetKernelArg(scan, 2, /*TODO:argsize*/, /*TODO:argval L int *current*/);
//    errNum |= clSetKernelArg(scan, 3, /*TODO:argsize*/, /*TODO:argval L int *buffered*/);

    /*Reorder*/
//    errNum  = clSetKernelArg(reorder, 0, /*TODO:argsize*/, /*TODO:argval int *input*/);
//    errNum |= clSetKernelArg(reorder, 1, /*TODO:argsize*/, /*TODO:argval int *output*/);
//    errNum |= clSetKernelArg(reorder, 2, /*TODO:argsize*/, /*TODO:argval int *scan*/);
//    errNum |= clSetKernelArg(reorder, 3, /*TODO:argsize*/, /*TODO:argval int *count*/);

    if(errNum != CL_SUCCESS){
        printf("counterror\n");
        exit(1);
    }


    size_t workGroupSize;
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), (void*)&workGroupSize, NULL);
    size_t global_work_size = (ARRLEN % workGroupSize == 0? ARRLEN / workGroupSize : ARRLEN / workGroupSize + 1) * workGroupSize;
    size_t local_work_size = workGroupSize;
    printf("GlobalWorkSize = %zd\nLocalWorkSize = %zd\n", global_work_size, local_work_size);

    errNum = clEnqueueNDRangeKernel(command_queue, count, 1, NULL, &global_work_size, /*&local_work_size*/NULL, 0, NULL, NULL);
    if(errNum != CL_SUCCESS){
        printf("clEnqueueNDRangeKernel error \n");
        exit(1);
    }

    clFinish(command_queue);


    int* outarray = malloc(sz);
    errNum = clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, sz, (void *)outarray, 0, NULL, NULL);

    for(i=0; i<ARRLEN; i++)
        printf("[%d]", array[i]);
    printf("\n\n");

    for(i=0; i<ARRLEN; i++)
        printf("[%d]", outarray[i]);
    printf("\n");
    return 0;
}
