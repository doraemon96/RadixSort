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
#define _RADIXSROT_H_

//Max source size for the kernels file (radixsort.cl)
#define MAX_SOURCE_SIZE 0x100000

//Number of kernels in the kernels file (radixsort.cl)
#define N_KERNELS 3

//Maximum length of a kernel in the kernels file (radixsort.cl)
#define MAX_KERNEL_NAME 20


/*Testing functions*/
//Size of the array to order (if _RS_FILLFUN_ not defined, generateArray will create a random one).
#define ARRLEN 256


#endif /*_RADIXSORT_H_*/
