#include <iostream>
#include <vector>
#include <string>
#include <omp.h>
#include <cstring>


using namespace std;

int cmp_func(const void *a , const void *b)
{
    return ( *(int *) a - *(int *) b );
}

int difference(int a[], int b[], int&n){
    
    int i;
    for(i = 0 ; i < n ; ++i){
        if(a[i] != b[i])
            return 0;
    }
    return 1;
}

void PrintVec (const int a[], int n )
{
  for (int i = 0; i <n ; ++i) cout << a[i] << "   ";

  cout << endl;

}


int main(int argc, char* args[])
{
  int i, j;
  int n, count;
  int numberOfThreads;
  double t_start, t_end, t_count_sort, t_quicksort;
  
  srand(time(NULL));
  
  // Get array size and number of threads from the command line                                       
  if (argc > 1) {
      n = strtol(args[1], NULL, 10);
      numberOfThreads = strtol(args[2], NULL, 10);
  }
  else {
      cout << "\nWhen running this program, please include the array size and the number of threads on command line.\n";
      return 0;
  }

  n = atoi(args[1]);
  cout <<  "n = " <<  n << "\n";
  
  int a[n];
  int b[n];
  int temp[n];
  
  for (i = 0; i < n; ++i) {
    b[i] = a[i] = rand() % 1000;
    temp[i] = 0;
  }  


    

    // count_sort starts here

    t_start = omp_get_wtime();
  
    // TODO: - Parallelize the i-loop and j-loop
    //       - You will turn in 2 different source code files for this problem:
    //         1. i-loop, 2. j-loop
    //         parallel versions.

    #pragma omp parallel for num_threads(numberOfThreads) private(count, i,j) shared(n)
    for(i = 0 ; i < n ; i++){
        count = 0;
        for(j = 0 ; j < n ; j++){
            if(a[j] < a[i])
                count++;
            else if((a[j] == a[i]) && (j < i))
                count++;
        }
        
        temp[count] = a[i];
    }
    


    
    // TODO: Modify the code below so that the copy can be made OpenMP parallel
    // memcpy(a, temp , n * sizeof(int));

    // Parallelized version 
    #pragma omp parallel for num_threads(numberOfThreads)
    for (int k = 0; k < n; ++k) a[k] = temp[k];


    t_end = omp_get_wtime();
    
    t_count_sort = t_end - t_start;
    
    cout << "Time needed for count sort using "<<  numberOfThreads << " threads = " << t_end - t_start << "\n"; 
    
    //count_sort ends here
    
    
    //quicksort starts
    

    t_start = omp_get_wtime();
    
    qsort(&b, n, sizeof(int), cmp_func);
    
    t_end = omp_get_wtime();
    
    t_quicksort = t_end - t_start ;
    
    cout << "time needed for sequential Quicksort = "<< t_quicksort <<"\n";
    


    //compare the results
       if (difference(a,b,n) == 0)
        cout << "Wrong Ans\n";
    else cout << "Correct Ans\n";

  return 0;
}
