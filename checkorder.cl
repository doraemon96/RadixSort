/*
 *                  CHECKORDER.CL
 *
 * "checkorder.cl" is a compendium of functions for the openCL
 * implementation of an order checking algorithm.
 *
 * 2016 Project for the "Facultad de Ciencias Exactas, Ingenieria
 * y Agrimensura" (FCEIA), Rosario, Santa Fe, Argentina.
 *
 * Implementation by Paoloni Gianfranco and Soncini Nicolas.
 */


/** PARALLEL COMPARE KERNEL **/
__kernel void parallelcmp(const __global int* input,
                    __global int* output,
                    const int nkeys)
{
    uint g_id = (uint) get_global_id(0);
    uint l_size = (uint) get_local_size(0);    
    uint n_groups = (uint) get_num_groups(0); 
    
    //Calculate elements to process per item
    int size = (nkeys / n_groups) / l_size;
    //Calculate where to start on the global array
    int start = g_id * size;

    //Set the output to 0
    int i;
    for(i = 0; i < size; i++) {
        output[i + start] = 0;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    for(i = 0; i < size; i++) {
        if (g_id == 0 && i == 0) {
            ;
        }
        else if (input[i + start] < input[i + start - 1])
            output[i + start] = 1;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
}

/** REDUCE COMPARE KERNEL **/
__kernel void reduce(__global int* input,
                     __global int* output,
                    const int nkeys)
{
    uint g_id = (uint) get_global_id(0);
    uint l_id = (uint) get_local_id(0);
    uint l_size = (uint) get_local_size(0);    

    uint group_id = (uint) get_group_id(0);
    uint n_groups = (uint) get_num_groups(0); 

    //UP SWEEP
    int d, offset = 1;
    for(d = l_size; d > 0; d >>= 1){
        barrier(CLK_LOCAL_MEM_FENCE);
        if(l_id < d) {
            int a = offset * (2 * l_id + 1) - 1;
            int b = offset * (2 * l_id + 2) - 1;
            input[b] += input[a];
        }
        offset *= 2;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    if(g_id == 0)
        output[0] = input[nkeys - 1];
}
