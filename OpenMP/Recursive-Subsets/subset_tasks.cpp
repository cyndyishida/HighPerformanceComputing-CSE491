#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <algorithm>
using namespace std;

vector<double> S; // this is the input vector
double global_min = 1000000000000.0;
double main_sum;
int thread_count;

void printVec(vector<double>&t)
{
    for(auto element: t) cout << element  << "  " ;
    cout << endl;
}

bool sameVec(vector<double> &a , vector<double> &b )
{
 	if (a.size() == b.size() ) 
 	{	
		sort(a.begin(), a.end());
		sort(b.begin(), b.end());
	
		for (int i = 0; i < a.size() ; ++i)
			if (a[i] != b[i])
				return false;
		return true;
		
	}

     return false;

} 


//index is the index of the new element to be added into S
void SerialGenerateSubset(int index, vector<double> &S, vector<double> subset) {
  
    if (index == S.size())
    {
     

   	double current_sum  = accumulate(subset.begin(), subset.end(), 0);
   // 	printVec(subset);

   	 if ( (abs(current_sum  - main_sum/2  )) < global_min ) global_min = abs(current_sum  - main_sum/2) ;
	return  ; 

    }
   

 
    subset.emplace_back(S[index]); 

  
    SerialGenerateSubset(index+1, S, subset);//generate subsets with the index element in them
     

    auto removeMe = find(subset.begin(), subset.end(), S[index]);
    subset.erase(removeMe);

    SerialGenerateSubset(index+1, S, subset); //generate subsets without the index element



}




//index is the index of the new element to be added into S
void ParallelGenerateSubset(int index, vector<double> &S, vector<double> subset) {
 


    if ( global_min  < 8 )  
	{

		SerialGenerateSubset(index, S, subset);
	
		return;
	}
 
    if (index == S.size())
    {
     

   	double current_sum  = accumulate(subset.begin(), subset.end(), 0);
   	//printVec(subset);

   	 if ( (abs(current_sum  - main_sum/2  )) < global_min ) global_min = abs(current_sum  - main_sum/2) ;
	return  ; 

    }
   
   
   { 
    subset.emplace_back(S[index]); 

        int count = omp_get_num_threads();
 
     	#pragma omp task if( count < 5 ) mergeable shared( S, global_min ) firstprivate(subset , index )
     	ParallelGenerateSubset(index+1, S, subset);//generate subsets with the index element in them
                                                                                                                      

  
    auto removeMe = find(subset.begin(), subset.end(), S[index]);
    subset.erase(removeMe);
    

         count = omp_get_num_threads();

  	#pragma omp task if( count < 5 ) mergeable shared(S, global_min ) firstprivate(subset)
   	 ParallelGenerateSubset(index+1, S, subset); //generate subsets without the index elemen





  }
}


int main(int argc, char* argv[]) {
  double start_time, end_time, time_diff;
  int i, nweights, nthreads;
  vector<double> subset; // this is where you will generate the subsets
  subset.clear();
 
  // Get the number of weights and number of threads from the command line
  if (argc == 3) {
    nweights = strtol(argv[1], NULL, 10);
    nthreads = strtol(argv[2], NULL, 10);
  }
  else {
    printf("\nWhen running this program, please include number of weights and number of threads on command line.\n");
    return 0;
  }
  
  thread_count = nthreads;
  printf("\nnumber of weigts: %d\n", nweights);
  printf("number of threads: %d\n", nthreads);
    
  main_sum = 0;
  //srand(time(NULL));
  srand(0);
  for (i = 0 ; i < nweights; i++) {
    S.push_back(((double)rand())/RAND_MAX);
    main_sum += S[i];
  }
  
  printf("main set : ");
  for (i = 0 ; i < S.size() ; i++)
    printf("%lf ", S[i]);
  printf("\n");
  printf("main sum = %lf\n", main_sum);
  
   
  start_time = omp_get_wtime();
 
  #pragma omp parallel num_threads(nthreads)

  #pragma omp single
  ParallelGenerateSubset(0, S, subset);
  
  end_time = omp_get_wtime();
  

  printf("\nParallel minimum diff = %.14lf\n", global_min);
  
  printf("parallel time needed = %f\n\n", end_time - start_time);

  global_min = 10000000000000000.01;

  start_time = omp_get_wtime();

  SerialGenerateSubset(0,S,subset);

  end_time = omp_get_wtime();

  printf("\nSerial minimum diff = %.14lf\n", global_min);
  
  printf("serial time needed = %f\n\n", end_time - start_time); 
}



