/*
 *                  RADIXSORT.CL
 *
 * "radixsort.cl" is a compendium of functions for the openCL
 * implementation of the Radix Sort algorithm.
 *
 * 2016 Project for the "Facultad de Ciencias Exactas, Ingenieria
 * y Agrimensura" (FCEIA), Rosario, Santa Fe, Argentina.
 *
 * Implementation by Paoloni Gianfranco and Soncini Nicolas.
 */


/*
 * COUNT
 */
#define GETRADIX(e,r) ((e >> r ) & 1 )  //Used to be casted to (unsigned int), shouldn't everything?
__kernel void count(__global int *input, __global int *output, int radix)
{
    uint global_id = get_global_id(0);

    //Obtain the current "radix" (bit)
    output[global_id] = GETRADIX(input[global_id], radix);
    //Invert the results
    output[global_id] = output[global_id]? 0 : 1 ;
}



/*
 * NAIVEDBPARALELLSCAN
 */
#define SWAP(a,b)  {__local int *tmp=a;a=b;b=tmp;}
__kernel void naivedbParallelScan(__global int *input, __global int *output, __local int *current, __local int *buffered)
{
    uint local_id = get_local_id(0);
    uint global_id = get_global_id(0);
    uint local_size = get_local_size(0);

    current[local_id] = buffered[local_id] = input[global_id];
    barrier(CLK_LOCAL_MEM_FENCE);

    int d;
    for(d=1; d<local_size; d*=2) {
        if(global_id > d) {
            current[local_id] = buffered[local_id] + buffered[local_id - d];
        }
        else {
            current[local_id] = buffered[local_id];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        SWAP(current, buffered);
    }
    output[global_id] = buffered[local_id];
}

/*
º__kernel void parallelScan(__global int *input, __global int *output, int n)
{

}
*/



/*
 * REORDER
 */
__kernel void reorder(__global int *input, __global int *output, __global int *scan, __global int *count)
{
    uint local_id = get_local_id(0);
    uint global_id = get_global_id(0);
    uint local_size = get_local_size(0);
    uint group_id = get_group_id(0);
    __local uint totalFalses;

    if(local_id == 0) { 
        int i = (group_id + 1)*local_size + 1;
        totalFalses = scan[i] + count[i];
    }

    int newPos;
    if(count[global_id]) { //If its radix is a zero
        newPos = global_id + scan[global_id];  
    }
    else {
        newPos = global_id - scan[global_id] + totalFalses;
    }

    output[newPos] = input[global_id];
}
