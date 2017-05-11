#include <iostream>
#include <stdlib.h>
#include <omp.h>

using namespace std;



// Constants
int RANDMAX = 1;
int RANDMIN = -1;
unsigned int SEED = 0;



double sum = 0.0;

// Method declarations
void* calculatePartialSum (void* threadNumber);
double sequentialCompute (long iterations);
double parallelCompute_atomic(long iterations, int numberOfThreads);
double parallelCompute_reduction(long iterations, int numberOfThreads);
double getDifference(double calculatedPi);

// Main method
int main(int argc, char* argv[])
{
  // Variable declarations
  double sequentialStart, sequentialEnd, sequentialTime;
  double parallelStart, parallelEnd, parallelTime_atomic, parallelTime_reduction;

  double sequentialPi, parallelPi_atomic, parallelPi_reduction;
  double sequentialDifference, parallelDifference_atomic, parallelDifference_reduction;
  long iterations; 
  int numberOfThreads;

  // Get number of iterations and number of threads from the command line
  if(argc > 1)
    {
      iterations = strtol(argv[1], NULL, 10);
      numberOfThreads = strtol(argv[2], NULL, 10);
    }
  else
    {
      cout <<"\nWhen running this program, please include number of iterations and number of threads on command line.\n";
      return 0;
    }

  // Time sequential calculation
  sequentialStart = omp_get_wtime();
  sequentialPi = sequentialCompute(iterations);
  sequentialEnd = omp_get_wtime();
  sequentialTime = sequentialEnd - sequentialStart;
  
  // Time parallel calculation with atomics
  parallelStart = omp_get_wtime();
  parallelPi_atomic = parallelCompute_atomic(iterations, numberOfThreads);
  parallelEnd = omp_get_wtime();
  parallelTime_atomic = parallelEnd - parallelStart;

  // Time parallel calculation with reduction
  parallelStart = omp_get_wtime();
  parallelPi_reduction = parallelCompute_reduction(iterations, numberOfThreads);
  parallelEnd = omp_get_wtime();
  parallelTime_reduction = parallelEnd - parallelStart;
  
  // How do results compare with PI?
  sequentialDifference = getDifference(sequentialPi);
  parallelDifference_atomic = getDifference(parallelPi_atomic);
  parallelDifference_reduction = getDifference(parallelPi_reduction);
  
  // Print results
  cout <<"Sequential Calculation: " <<  sequentialPi << "\n";
  cout <<"This is " <<  sequentialDifference << " away from the correct value of PI.\n\n";
  cout <<"ParallelCalculation with atomics: " << parallelPi_atomic  << "\n";
  cout <<"This is " << parallelDifference_atomic<< " away from the correct value of PI.\n";
  cout <<"Number of iterations:" <<  iterations << " Number of Threads:" <<  numberOfThreads << "\n";
  
  // Calculate the validity of the parallel computation
  double difference = parallelDifference_atomic - sequentialDifference;

  // if (difference < .01 && difference > -.01)
  if (parallelDifference_atomic < .01 && parallelDifference_atomic > -.01)
    cout <<"Parallel atomic calculation is VALID!\n";
  else
    cout <<"Parallel atomic calculation is INVALID!\n";

  // Calculate and print speedup results
  double speedup = ((double)sequentialTime)/((double)parallelTime_atomic);
  cout <<"Sequential Time: " << sequentialTime << " Parallel Atomic Time: " << parallelTime_atomic  << ", Speedup: " <<  speedup   << "\n";


  cout <<"\n\nParallelCalculation with reductions: "<< parallelPi_reduction<< "\n";
  cout <<"This is "<<parallelDifference_reduction<<" away from the correct value of PI.\n";
  cout <<"Number of iterations: "<< iterations << " Number of Threads: "<< numberOfThreads <<"\n";
  
  // Calculate the validity of the parallel computation
  difference = parallelDifference_reduction - sequentialDifference;

  //if (difference < .01 && difference > -.01)
  if (parallelDifference_reduction < .01 && parallelDifference_reduction > -.01)
    cout <<"Parallel reduction calculation is VALID!\n";
  else
    cout <<"Parallel reduction calculation is INVALID!\n";

  // Calculate and print speedup results
  speedup = ((double)sequentialTime)/((double)parallelTime_reduction);
  cout <<"Sequential Time: " << sequentialTime << " Parallel Reduction Time: " << parallelTime_reduction<< " Speedup: "<< speedup << "\n";

  return 0;
}


// TODO: You need to implement a sequential estimation for PI using the Monte Carlo method.
//       Use rand_r() here as well for fair comparison to OpenMP parallel versions.
double sequentialCompute (long iterations)
{   

    int numCircle = 0;

    for (int i = 0; i < iterations; ++i)
    {
        double x = RANDMIN + (double)(rand_r(&SEED)) /( (double)(RAND_MAX/(RANDMAX-RANDMIN)));
        double y = RANDMIN + (double)(rand_r(&SEED)) /( (double)(RAND_MAX/(RANDMAX-RANDMIN)));

        double distanceSquare = x*x + y*y;
        if (distanceSquare <= 1) ++numCircle;
    }

    return  4 * numCircle / (double) iterations ;
}


// Find how close the calculation is to the actual value of PI
double getDifference(double calculatedPi)
{
  return calculatedPi - 3.14159265358979323846;
}


// TODO: You need to implement an OpenMP parallel version using atomics
//       Use rand_r() for thread safe random number generation.
//       More details about rand_r() is here: http://linux.die.net/man/3/rand_r
//       You must also make sure that each thread start with a DIFFERENT SEED!
double parallelCompute_atomic(long iterations, int numberOfThreads)
{
 double numCircle = 0;

    double distanceSquare, x ,y;
    unsigned int i;


    #pragma omp parallel for num_threads(numberOfThreads)
    for (i = 0; i < iterations; ++i)
    {
        double x = RANDMIN + (double)(rand_r(&i)) /( (double)(RAND_MAX/(RANDMAX-RANDMIN)));
        double y = RANDMIN + (double)(rand_r(&i)) /( (double)(RAND_MAX/(RANDMAX-RANDMIN)));

        #pragma omp atomic
        x *=x;
        #pragma omp atomic
        y*=y;

        double distanceSquare = 0;
        #pragma omp atomic
        distanceSquare += (x + y);

        if (distanceSquare <= 1) 
        {
            #pragma omp atomic
            ++numCircle;
        }
            
            
    }

    return 4 * numCircle / (double) iterations ;
}


// TODO: Same as the other OpenMP version above,
// but uses OpenMP reduction clause to aggregate partial results
double parallelCompute_reduction(long iterations, int numberOfThreads)
{
    double numCircle = 0;
    double distanceSquare, x ,y;
    unsigned int i;
    #pragma omp parallel for num_threads(numberOfThreads) reduction( +: numCircle) private(i, x,y,distanceSquare) shared(iterations)

    for (i = 0; i < iterations; ++i)
    {
        x = RANDMIN + (double)(rand_r(&i)) /( (double)(RAND_MAX/(RANDMAX-RANDMIN)));
        y = RANDMIN + (double)(rand_r(&i)) /( (double)(RAND_MAX/(RANDMAX-RANDMIN)));

        distanceSquare = x*x + y*y;
        if (distanceSquare <= 1) numCircle++;
    }

    return 4 * numCircle / (double) iterations ;
    
}

