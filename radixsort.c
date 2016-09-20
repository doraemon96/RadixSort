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

//Funcion para determinar tamaño de archivo (desde la pos. actual)
int filesize(FILE *fp) {
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, prev, SEEK_SET); //volver a donde estaba
    return size;
}


int main() {

    //-------------------------
    // Inicializar data en host
    //-------------------------
    int *array = NULL; //Arreglo input
    int *output = NULL; //Arreglo output
    
    //Tamaño de la data del arreglo
    size_t array_dataSize = sizeof(int)*ARRLEN;

    //Allocar espacio de arreglos
    array = (int*)malloc(array_dataSize);
    output = (int*)malloc(array_dataSize);

    //Inicializar data en el arreglo
    array = _RS_FILLFUN_;

    cl_int errNum;

    //-----------------
    // Importar kernels
    //-----------------
    FILE *fp;
    const char file_name[] = KERNELS_FILENAME; /*TODO: AGREGAR AL .h*/
    size_t file_sourceSize;
    char *file_sourceStr;

    fp = fopen(file_name, "r");
    if(!fp) {
        printf("Error al abrir archivo de kernels: [%s]\n", file_name);
        exit(1);
    }
    file_sourceSize = filesize(fp); //TODO: Ojo con este casteo
    file_sourceStr = (char*)malloc(file_sourceSize);
    if(file_sourceSize != fread(file_sourceStr, 1, file_sourceSize, fp)) {
        printf("Error de lectura en archivo de kernels: [%s]\n", file_name);
        exit(1);
    }
    fclose(fp);


    //-------------------------
    // Obtener info plataformas
    //-------------------------
    cl_uint numPlatforms = 0;
    cl_platform_id *platforms = NULL;

    //Obtener numero de plataformas (mockcall)
    errNum = clGetPlatformIDs(0, NULL, &numPlatforms);
    //Allocar espacio por plataforma
    platforms = (cl_platform_id*)malloc(numPlatforms*sizeof(cl_platform_id));
    //Llenar con info de plataformas
    errNum = clGetPlatformIDs(numPlatforms, platforms, NULL);

    //---------------------
    // Obtener info devices
    //---------------------
    cl_uint numDevices = 0;
    cl_device_id *devices = NULL;

    //Obtener numero de device (mockcall)
    errNum = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL /*TODO GPU*/, 0, NULL, &numDevices);
    //Allocar espacio por device
    devices = (cl_device_id*)malloc(numDevices*sizeof(cl_device_id));
    //Llenar con info de plataformas
    errNum = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL /*TODO ^^^*/, numDevices, devices, NULL);

    //---------------
    // Crear contexto
    //---------------
    cl_context context = NULL;

    //Crear contexto asociado a los devices
    context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &errNum);

    //-----------------------
    // Crear cola de comandos
    //-----------------------
    cl_command_queue commandQueue;
    
    commandQueue = clCreateCommandQueue(context, devices[0], 0, &errNum);

    //--------------
    // Crear buffers
    //--------------
    cl_mem array_buffer;
    cl_mem output_buffer;

    //Crear buffer de input
    array_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY /*TODO*/, array_dataSize, NULL, &errNum);
    //Crear buffer de output
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, array_dataSize, NULL, &errNum);

    //---------------------------
    // Encolar escritura a device (host -> device buffer)
    //---------------------------
    
    errNum = clEnqueueWriteBuffer(commandQueue, array_buffer, CL_FALSE /*TODO*/, 0, array_dataSize, array, 0, NULL, NULL);

    //-----------------------------
    // Crear y compilar el programa
    //-----------------------------
    cl_program program = NULL;
    
    //Creacion de programa desde archivo
    program = clCreateProgramWithSource(context, 1, (const char**)&file_sourceStr, (const size_t*)&file_sourceSize, &errNum);
    //Compilado para openCL 1.1
    errNum = clBuildProgram(program, 1, devices, "-cl-std=CL1.1", NULL, NULL);

    //--------------
    // Crear kernels
    //--------------
    cl_kernel *kernels = (cl_kernel*)malloc(sizeof(cl_kernel)*KERNELS_NUMBER); //Tenemos mas de un kernel en el archivo
    
    //Tomar todos los kernels que encuentre
    errNum = clCreateKernelsInProgram(program, KERNELS_NUMBER, kernels, NULL);
    
    //Separar kernels
    cl_kernel count, scan, reorder;
    char kernelName[MAX_KERNEL_NAME];
    int i;
    for(i=0; i < KERNELS_NUMBER; i++){
        errNum = clGetKernelInfo(kernels[i], CL_KERNEL_FUNCTION_NAME, sizeof(kernelName), kernelName, NULL);
        if(errNum != CL_SUCCESS){
            printf("Error al renombrar kernels. Usando \"clGetKernelInfo\"\n");
            exit(1);
        }
        if(strcmp(kernelName, "count") == 0) 
            count = kernels[i];
        if(strcmp(kernelName, "naivedbParallelScan") == 0) 
            scan = kernels[i];
        if(strcmp(kernelName, "reorder") == 0) 
            reorder = kernels[i];
    }   
    
    if((count == NULL) || (scan == NULL) || (reorder == NULL)){
        printf("ERROR");
        exit(1);
    }

    //----------------------------
    // Setear argumentos a kernels
    //----------------------------

    //Argumentos a Count
    int bit = 0;
    errNum = clSetKernelArg(count, 0, sizeof(cl_mem), (void*)&array_buffer);   // Input array
    errNum |= clSetKernelArg(count, 1, sizeof(cl_mem), (void*)&output_buffer); // Output array
    errNum |= clSetKernelArg(count, 2, sizeof(int), (void*)&bit);       // Whatever

    //Argumentos a Scan
    errNum = clSetKernelArg(scan, 0, sizeof(cl_mem), (void*)&array_buffer);   // Input array
    errNum |= clSetKernelArg(scan, 1, sizeof(cl_mem), (void*)&output_buffer); // Output array
    errNum |= clSetKernelArg(scan, 2, array_dataSize, NULL); // current array
    errNum |= clSetKernelArg(scan, 3, array_dataSize, NULL); // buffered array


    //Argumentos a Reorder


    //------------------------------------
    // Configurar estructura de work-items
    //------------------------------------
    size_t globalWorkSize[1];
    
    globalWorkSize[0] = ARRLEN;

    //-------------------------------
    // Encolar kernels para ejecucion
    //-------------------------------
    
    errNum = clEnqueueNDRangeKernel(commandQueue, count, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    errNum = clEnqueueNDRangeKernel(commandQueue, scan, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);

    //-----------------------
    // Encolar lectura a host (device buffer -> host)
    //-----------------------
    
    clEnqueueReadBuffer(commandQueue, output_buffer, CL_TRUE, 0, array_dataSize, output, 0, NULL, NULL);
    
    //Verificar el output
    //bool result = true;
    //TODO VERIFICADOR DE ORDEN


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

    //------------------
    // Liberar resources
    //------------------
    //Propias de openCL
    clReleaseKernel(count); clReleaseKernel(scan); clReleaseKernel(reorder);
    clReleaseProgram(program);
    clReleaseCommandQueue(commandQueue);
    clReleaseMemObject(array_buffer); clReleaseMemObject(output_buffer);
    clReleaseContext(context);
    //Propias del host
    free(array); free(output);
    free(platforms);
    free(devices);
    
    return 0;
}
