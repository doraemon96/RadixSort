/*
 *                   RADIXSORT.H
 *
 * "radixsort.h" is the include file for the openCL
 * implementation of the Radix Sort algorithm.
 *
 * 2016 Project for the "Facultad de Ciencias Exactas, Ingenieria
 * y Agrimensura" (FCEIA), Rosario, Santa Fe, Argentina.
 *
 * Implementation by Paoloni Gianfranco and Soncini Nicolas.
 */


#ifndef _RADIXSORT_H_
#define _RADIXSORT_H_

//Kernel file name
#define KERNELS_FILENAME "radixsort.cl"

//Max source size for the kernels file (radixsort.cl)
#define MAX_SOURCE_SIZE 0x100000

//Number of kernels in the kernels file (radixsort.cl)
#define KERNELS_NUMBER 3

//Maximum length of a kernel in the kernels file (radixsort.cl)
#define MAX_KERNEL_NAME 20

#define WG_SIZE 64
#define N_GROUPS 16
#define BITS 32
//Number of buckets necessary
#define BUCK (1 << RADIX)
//Number of bits in the radix
#define RADIX 1

/*Testing functions*/
//Size of the array to order (if _RS_FILLFUN_ not defined, generateArray will create a random one).
#define ARRLEN 512

/*#define _RS_FILLFUN_ arraywithones()
int* arraywithones(void)
{
    int i, *array = malloc(sizeof(int) * ARRLEN);
    for(i=0; i<ARRLEN; i++)
        array[i] = 1;

    return array;
}
*/
#endif /*_RADIXSORT_H_*/
