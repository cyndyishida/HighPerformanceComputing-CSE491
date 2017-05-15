// diffusion.cu
//
// Program to model 1D heat diffusion equation (CUDA)

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#include <cuda.h>


float* diffusion_1D(float *a, float *b, long int size, long int time);
float* diffusion_2D(float *a, float *b, long int size, long int time);


int main (int argc, char *argv[])
{
    float *temp;
    float *array1, *array2;
    long int i, j, rodsize, tempsteps;
    long int index;
    int dim;
    struct timeval start, finish;
    char out_file[100];
    FILE *fp;

    if (argc != 5)
    {
        printf("usage: ./diffusion partitions tempsteps dimensions outfile\n");
        printf("    partitions: number of discrete points along a dimension\n");
        printf("    tempsteps: number of time steps to numerically estimate diffusion\n");
        printf("    dimensions: number of dimensions to perform the diffusion (1 or 2)\n");
        printf("    outfile: name of the output file for the diffusion results\n");
        exit(-1);
    }

    rodsize = atol(argv[1]);
    tempsteps = atol(argv[2]);
    dim = atoi(argv[3]);
    strncpy(out_file, argv[4], 100);

    fp = fopen(out_file, "w");
    if (ferror(fp) || fp == NULL)
    {
        fprintf(stderr, "ERROR: cannot open file %s for printing. Terminating...\n", out_file);
        exit(-1);
    }

    if (dim == 1)
    {
        array1 = (float *) malloc((rodsize + 2) * sizeof (float));
        array2 = (float *) malloc((rodsize + 2) * sizeof (float));

        /* populate the 1D grid with initial temperature values */
        array1[0] = 100.0;
        array2[0] = 100.0;
        for (i = 1; i <= rodsize; i++)
        {
            array1[i] = 23.0;
            array2[i] = 23.0;
        }
        array1[rodsize + 1] = 23.0;
        array2[rodsize + 1] = 23.0;

        /* perform numeric estimation of the 1D diffusion over time */
        gettimeofday(&start, NULL);
        temp = diffusion_1D(array1, array2, rodsize, tempsteps);
        gettimeofday(&finish, NULL);

        /* print diffusion info at a particular point */
        index = (int) (0.7 * rodsize);
        printf("Temperature at location 0.7 at time %ld: %7.2f\n", tempsteps, temp[index]);
        printf("Elapsed time: %f sec.\n\n", (finish.tv_sec - start.tv_sec)
                + (finish.tv_usec - start.tv_usec) / 1000000.0);

        /* write results to file */
        for (i = 1; i <= rodsize; ++i)
        {
            fprintf(fp, "%7.2f\n", temp[i]);
        }

        free(array2);
        free(array1);
    }

    if (dim == 2)
    {
        array1 = (float *) malloc((rodsize + 2) * (rodsize + 2) * sizeof(float));
        array2 = (float *) malloc((rodsize + 2) * (rodsize + 2) * sizeof(float));

        /* populate the 2D grid with initial temperature values */
        for (j = 0; j < rodsize + 2; j++)
        {
            array1[j] = 100.0;
            array2[j] = 100.0;
        }
        for (i = 1; i < rodsize + 2; i++)
        {
            for (j = 0; j < rodsize + 2; j++)
            {
                array1[i * (rodsize + 2) + j] = 23.0;
                array2[i * (rodsize + 2) + j] = 23.0;
            }
        }

        /* perform numeric estimation of the 2D diffusion over time */
        gettimeofday(&start, NULL);
        temp = diffusion_2D(array1, array2, rodsize, tempsteps);
        gettimeofday(&finish, NULL);

        /* print diffusion info at a particular point */
        index = (int) (0.7 * rodsize);
        printf("Temperature at location 0.7 at time %ld: %7.2f\n", tempsteps, temp[index]);
        printf("Elapsed time: %f sec.\n\n", (finish.tv_sec - start.tv_sec)
                + (finish.tv_usec - start.tv_usec) / 1000000.0);

        for (i = 1; i <= rodsize; ++i)
        {
            for (j = 1; j <= rodsize; ++j)
            {
                fprintf(fp, "%7.2f ", temp[i * (rodsize + 2) + j]);
            }
            fprintf(fp, "\n");
        }

        free(array2);
        free(array1);
    }

    fclose(fp);

    return 0;
}


__global__ void diffusion_1D_kernel(float *a, float *b, long int size)
{

    int k = blockIdx.x * blockDim.x + threadIdx.x;
    if (k != 0  && k < size)
        a[k] = (b[k - 1] + b[k + 1]) / 2.0;
    
    if (k==0)
        a[size + 1] = a[size];
}


/* Compute the diffusion over a 1D grid over the prescribed time
 *
 * a/b: arrays containing the current and next temperatures over the 1D grid
 *  note: b must contain the initial grid values
 * size: total number of elements in the 2D grid
 * time: number of time steps for which to diffuse (arbitrary units)
 */
float* diffusion_1D(float *a, float *b, long int size, long int time)
{

    cudaError err;
    
    int siz = (size + 2) * sizeof(float);     
    float *d_A, *d_B;    
    float *temp = (float*) malloc(siz);
    
    err = cudaMalloc((void **) &d_A, siz);
    if (err){
        printf("Error = %s\n", cudaGetErrorString(err));
    }
    
    
    
    cudaMemcpy(d_A, a, siz, cudaMemcpyHostToDevice);

      

      
    cudaMalloc((void **) &d_B, siz);
    
    cudaMemcpy(d_B, b, siz, cudaMemcpyHostToDevice);
    if (err)
        printf("Error =  %s\n", cudaGetErrorString(err));

    int numBlocks = ceil(size / 256.0);
    int i = 0;
    float *tempPtr;
    dim3 threadsPerBlock(size);    
    for (i=0; i<time; i++){
        diffusion_1D_kernel<<<numBlocks, threadsPerBlock>>>(d_A, d_B, size);
        
        tempPtr = d_A;
        d_A = d_B;
        d_B = tempPtr;
        
    }
    

    cudaThreadSynchronize();
    
    
    cudaMemcpy(a, d_A, size, cudaMemcpyDeviceToHost);   

    cudaFree(d_A); 
    cudaFree(d_B);
    
    return a;
}


__global__ void diffusion_2D_kernel(float *a, float *b, long int size)
{

    long int i, j, k, siz;

    siz = (size + 2);
    i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i != 0  && i < size)
        for(j = 1; j <= size; j++)
            for (k = 1; k <= size; k++)
                a[j*siz + k] = (b[j*siz + k-1] + b[j*siz + k + 1] + b[(j-1) * siz + k] + b[(j+1)*siz +k]) / 4.0;
            
        
    

    if(i == 0)
        a[(size+1)*siz] = a[size*siz];


}


/* Compute the diffusion over a 2D grid over the prescribed time
 *
 * a/b: arrays containing the current and next temperatures over the 2D grid
 *  note: arrays will be linearized (flattened) from 2D to 1D
 *  note: b must contain the initial grid values
 * size: total number of elements in the 2D grid
 * time: number of time steps for which to diffuse (arbitrary units)
 */
float* diffusion_2D(float *a, float *b, long int size, long int time){

    float *d_A, *d_B; 
    
    int siz = (size + 2)* (size+2) * sizeof(float);         
    float *temp = (float*) malloc(siz);
    
    cudaMalloc((void **) &d_A, siz);

    cudaMemcpy(d_A, a, siz, cudaMemcpyHostToDevice);
      
    cudaMalloc((void **) &d_B, siz);
    
    cudaMemcpy(d_B, b, siz, cudaMemcpyHostToDevice);

    dim3 threadsPerBlock(size);
    int numBlocks = ceil(size / 256.0);
    int r = 0;
    for(r = 0; r < time; r++)
    {
        /* compute next grid values (a = next, b = current) */
        diffusion_2D_kernel<<<numBlocks, threadsPerBlock>>>(d_A, d_B, size);

        cudaThreadSynchronize();
        
        /* swap current and new grid points */
        temp = d_A;
        d_A = d_B;
        d_B = temp;
    }


    cudaThreadSynchronize();
    
    cudaMemcpy(a, d_A, siz, cudaMemcpyDeviceToHost);   

    cudaFree(d_A); 
    cudaFree(d_B);

    return a;
}
