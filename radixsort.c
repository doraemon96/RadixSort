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

#include <CL/cl.h>

int main(void){

    cl_uint errNum;

    FILE *fp;
    const char fileName[] = "./RadixSort.cl";
    size_t source_size;
    char *source_str;

    fp = fopen(fileName, "r");
    if(!fp){
        printf("Error al cargar \"%s\"", fileName);
        return(1);
    }
    source_str = (char *)malloc(MAX_SOURCE_SIZE);               /*Podria hacer algo mas descriptivo que un DEFINE*/
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);    /*Lee a source_str y deja en source_size la cantidad de bytes(o chars?) leidos*/
    fclose(fp);

    /*Tomar info de plataformas y devices*/
    cl_platform_id platform_id;
    cl_uint num_platforms;
    cl_device_id device_id;
    cl_uint num_devices

    errNum = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    errNum = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &num_devices);

    /*Crear contexto y cola de comandos*/
    cl_context context;
    cl_command_queue command_queue;
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &errNum);
    command_queue = clCreateCommandQueue(context, device_id, NULL, &errNum);

    /*Crear kernels desde el source */
    cl_program program = NULL;
    program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t *)&source_size, &errNum);
    
    /*Crear kernel desde programa*/
    errNum = 
    
