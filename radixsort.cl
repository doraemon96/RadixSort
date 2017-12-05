/*
 *                  RADIXSORT.CL
 *
 * "radixsort.cl" is a compendium of functions for the openCL
 * implementation of the Radix Sort algorithm.
 *
 * 2017 Project for the "Facultad de Ciencias Exactas, Ingenieria
 * y Agrimensura" (FCEIA), Rosario, Santa Fe, Argentina.
 *
 * Implementation by Paoloni Gianfranco and Soncini Nicolas.
 */

#include  <radixsort.h>

/** COUNT KERNEL **/

__kernel void count(const __global int* input,
                    __global int* output,
                    __local int* local_histo,
                    const int pass,
                    const int nkeys)
{
    uint g_id = (uint) get_global_id(0);
    uint l_id = (uint) get_local_id(0);
    uint l_size = (uint) get_local_size(0);    

    uint group_id = (uint) get_group_id(0);
    uint n_groups = (uint) get_num_groups(0); 
    
    //Set the buckets of each item to 0
    int i;
    for(i = 0; i < BUCK; i++) {
        local_histo[i * l_size + l_id] = 0;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    //Calculate elements to process per item
    int size = (nkeys / n_groups) / l_size;
    //Calculate where to start on the global array
    int start = g_id * size;
    
    for(i = 0; i < size; i++) {
        int key = input[i + start];
        //Extract the corresponding radix of the key
        key = ((key >> (pass * RADIX)) & (BUCK - 1));
        //Count the ocurrences in the corresponding bucket
        local_histo[key * l_size + l_id]++;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    for(i = 0; i < BUCK; i++) {
        //"from" references the local buckets
        int from = i * l_size + l_id;
        //"to" maps to the global buckets
        int to = i * n_groups + group_id;
        //Map the local data to its global position
        output[l_size * to + l_id] = local_histo[from];
    }
    
    barrier(CLK_GLOBAL_MEM_FENCE);
}


/** SCAN KERNEL **/
__kernel void scan(__global int* input,
                   __local int* local_scan,
                   __global int* block_sum)
{
    uint g_id = (uint) get_global_id(0);
    uint l_id = (uint) get_local_id(0);
    uint l_size = (uint) get_local_size(0);    

    uint group_id = (uint) get_group_id(0);
    uint n_groups = (uint) get_num_groups(0); 

    //Store data from global to local memory to operate
    local_scan[2 * l_id] = input[2 * g_id];
    local_scan[2 * l_id + 1] = input[2 * g_id + 1];

    //UP SWEEP
    int d, offset = 1;
    for(d = l_size; d > 0; d >>= 1){
        barrier(CLK_LOCAL_MEM_FENCE);
        if(l_id < d) {
            int a = offset * (2 * l_id + 1) - 1;
            int b = offset * (2 * l_id + 2) - 1;
            local_scan[b] += local_scan[a];
        }
        offset *= 2;
    }
    
    if (l_id == 0) {
        //Store the full sum on last item
        if(block_sum != NULL){
            block_sum[group_id] = local_scan[l_size * 2 - 1];
        }

        //Clear the last element
        local_scan[l_size * 2 - 1] = 0;
    }

    //DOWN SWEEP
    for(d = 1; d < (l_size*2); d *= 2) {
        offset >>= 1;
        barrier(CLK_LOCAL_MEM_FENCE);
        if(l_id < d) {
            int a = offset * (2 * l_id + 1) - 1;
            int b = offset * (2 * l_id + 2) - 1;
            int tmp = local_scan[a];
            local_scan[a] = local_scan[b];
            local_scan[b] += tmp;
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    //Write results from Local to Global memory
    input[2 * g_id]     = local_scan[2 * l_id];
    input[2 * g_id + 1] = local_scan[2 * l_id + 1];
}


/** COALESCE KERNEL **/
__kernel void coalesce(__global int* scan,
                       __global int* block_sums)
{

    uint g_id = (uint) get_global_id(0);
    uint group_id = (uint) get_group_id(0);

    int b = block_sums[group_id];

    //TODO: Probar pasar a memoria local
    scan[2 * g_id] += b;
    scan[2 * g_id + 1] += b;

    barrier(CLK_GLOBAL_MEM_FENCE);
}
