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
#include "createarray.c"
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

    /*Crear programa de kernels desde el source */
    cl_program program = NULL;
    program = clCreateProgramWithSource(context, 1, (const char**)&source_str, (const size_t *)&source_size, &errNum);
    
    /*Compilar programa*/
    errNum = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    
    /*Crear kernel*/
    cl_kernel *kernels = (cl_kernel*)malloc(N_KERNELS * sizeof(cl_kernel));
    errNum = clCreateKernelsInProgram(program, N_KERNELS, kernels, NULL); //errNum es uint y esto devuelve int, posible error de compilación?

    /*Separar kernels (con nombre)*/
    cl_kernel count, scan, reorder;
    char kernelName[MAX_KERNEL_NAME];
    int i;
    for(i=0, i < N_KERNELS, i++){
        errNum = clGetKernelInfo(kernels[i], CL_KERNEL_FUNCTION_NAME, sizeof(kernelName), kernelName, NULL);
        if(errNum != CL_SUCCESS){
            //Error!
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
        //Error!
    }
    

    /*Buffers*/
    cl_uint max_cunits = 0;
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), (void*)&max_cunits, NULL);
    int N = retN();
    size_t sz = sizeof(int)*N; 
    cl_mem input = (context, CL_MEM_READ_WRITE, sz), )    

    /*Setear parametros a kernels*/
    /*Count*/
    errNum  = clSetKernelArg(count, 0, /*TODO:argsize*/, /*TODO:argval int *input*/);
    errNum |= clSetKernelArg(count, 1, /*TODO:argsize*/, /*TODO:argval int *output*/);
    errNum |= clSetKernelArg(count, 2, /*TODO:argsize*/, /*TODO:argval int radix*/);

    /*Scan*/
    errNum  = clSetKernelArg(scan, 0, /*TODO:argsize*/, /*TODO:argval int *input*/);
    errNum |= clSetKernelArg(scan, 1, /*TODO:argsize*/, /*TODO:argval int *output*/);
    errNum |= clSetKernelArg(scan, 2, /*TODO:argsize*/, /*TODO:argval L int *current*/);
    errNum |= clSetKernelArg(scan, 3, /*TODO:argsize*/, /*TODO:argval L int *buffered*/);

    /*Reorder*/
    errNum  = clSetKernelArg(reorder, 0, /*TODO:argsize*/, /*TODO:argval int *input*/);
    errNum |= clSetKernelArg(reorder, 1, /*TODO:argsize*/, /*TODO:argval int *output*/);
    errNum |= clSetKernelArg(reorder, 2, /*TODO:argsize*/, /*TODO:argval int *scan*/);
    errNum |= clSetKernelArg(reorder, 3, /*TODO:argsize*/, /*TODO:argval int *count*/);

    if(errNum != CL_SUCCESS){
        //Error!
    }

}
