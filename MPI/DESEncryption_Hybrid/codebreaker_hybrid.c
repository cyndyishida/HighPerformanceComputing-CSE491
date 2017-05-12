#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>


#define ROTL32(x,y) ((x<<y)|(x>>(32-y)))
#define ROTR32(x,y) ((x>>y)|(x<<(32-y)))
#define ROTL24(x,y) ((x<<y)|(x>>(24-y)))
#define ROTR24(x,y) ((x>>y)|(x<<(24-y)))
#define ROTL16(x,y) ((x<<y)|(x>>(16-y)))
#define ROTR16(x,y) ((x>>y)|(x<<(16-y)))
#define ROTL8(x,y) ((x<<y)|(x>>(8-y)))
#define ROTR8(x,y) ((x>>y)|(x<<(8-y)))

int num_entries = 8;
const char *dictionary[] = {"bird", "campus", "class", "of", 
			    "spring", "sun", "the", "tree"}; 


// checks if a significant portion of the words resulting from 
// the decryption with the tried key are found in the dictionary 
int isValid(char *decoded, int len)
{
  int i, flag;
  int nmatches = 0;
  char *word;
  
  word = strtok(decoded," ,.;-()\n\r");
  while (word != NULL) { 
    flag = 0;
    for (i=0; i < num_entries; ++i) {
      if(strcmp(word, dictionary[i]) == 0) {
	flag = 1;
	break;
      }	
    }
    if (flag)
    {
      nmatches += strlen(word);
      //printf("MATCH: %s (%d)\n",word,strlen(word));
    }
    word = strtok(NULL," ,.;-()\n\r");
  }

  // different criteria may be used for deciding whether the tried 
  // key was a valid one. here we identify it as valid if words in 
  // the decrypted message that can be located in the dictionary account 
  // for more than half of the original message length.
  if (nmatches > len * 0.50)
    return 1;

  return 0;
}


void decrypt32(unsigned char *inp, unsigned key, unsigned char *decoded, unsigned len)
{
  int iend, oend;
  unsigned block;
  unsigned a, b, c, d, magnitude, polarity, xor;
  unsigned remaining;
  
  srand(key);
  
  remaining = len;
  iend = 0;
  oend = 0;        
  
  /* main loop for decoding -- all 4 bytes are valid */
  while(remaining >= 4) {
    a = inp[iend++];
    b = inp[iend++];
    c = inp[iend++];
    d = inp[iend++];

    //printf("a = %x, b = %x, c = %x, d=%x\n", a, b, c, d);
    polarity = rand()%2;
    magnitude = rand()%32;
    block = ((d<<24) | (c<<16) |(b<<8) | a);
    
    if (polarity) 
      block = ROTR32(block,magnitude);
    else block = ROTL32(block,magnitude);
    
    xor = ((rand()%256<<24) | (rand()%256<<16) | 
	   (rand()%256<<8) | rand()%256);
    block ^= xor;

    decoded[oend++] = block;
    decoded[oend++] = (block=block>>8);
    decoded[oend++] = (block=block>>8);
    decoded[oend++] = (block=block>>8);
    //printf("p = %d, mag = %d, xor = %d\n", polarity, magnitude, xor);

    remaining -= 4;
  }
  
  /* end cases */
  if (remaining == 3) {
    a = inp[iend++];
    b = inp[iend++];
    c = inp[iend++];

    polarity = rand()%2;
    magnitude = rand()%24;
    block = ((c<<16) |(b<<8) | a);
    
    if (polarity) 
      block = ROTR24(block,magnitude);
    else block = ROTL24(block,magnitude);

    xor = ((rand()%256<<16) | (rand()%256<<8) | rand()%256);
    block ^= xor;

    decoded[oend++] = block;
    decoded[oend++] = (block=block>>8);
    decoded[oend++] = (block=block>>8);
  }
  else if (remaining == 2) {
    a = inp[iend++];
    b = inp[iend++];

    polarity = rand()%2;
    magnitude = rand()%16;
    block = ((b<<8) | a);
    
    if (polarity) 
      block = ROTR16(block,magnitude);
    else block = ROTL16(block,magnitude);

    xor = ((rand()%256<<8) | rand()%256);
    block ^= xor;
    
    decoded[oend++] = block;
    decoded[oend++] = (block=block>>8);
  }
  else if (remaining == 1) {
    a = inp[iend++];

    polarity = rand()%2;
    magnitude = rand()%8;
    block = (a);
    
    if (polarity) 
      block = ROTR8(block,magnitude);
    else block = ROTL8(block,magnitude);
    
    xor = (rand()%256);
    block ^= xor;
    
    decoded[oend++] = block;
  }
}


void decrypt8(unsigned char *inp, unsigned key, unsigned char *decoded, unsigned len)
{
  int iend, oend;
  unsigned char a, magnitude, block, xor;
  unsigned remaining;
  
  remaining = len;
  iend = 0;
  decoded[0] = 0; // C strings are zero-terminated
  oend = 0;        

  srand(key);
  
  /* main loop for encoding */
  while(remaining > 0) {
    a = inp[iend++];
    magnitude = rand()%8;
    xor = rand()%256;
    block = a;
    block = ROTR8(block, magnitude);
    block ^= xor;
    decoded[oend++] = block;
    decoded[oend] = 0;
//    printf("%x --> %x, mag: %d, xor: %x\n", a, block, magnitude, xor);

    --remaining;
  }

}




int main(int argc,char *argv[])
{
  char a;
  char outfilename[100];
  unsigned char encrypted[1000], decrypted[1000], dcopy[1000];
  FILE *fin, *fout;
  int success = 0;
  unsigned len, comm_sz, my_rank;
  double tstart, tend;
  unsigned long long  i;


	// Parallelzing initalization
	MPI_Init(NULL,NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);


  printf("\n\nx0r 32-bit code breaker\n\n");

  if (argc == 1) {
    fprintf(stderr, "ERROR: No file(s) supplied.\n");
    fprintf(stderr, "USAGE: This program requires a filename \
                     to be provided as argument!");
    exit(1);
  }

 // printf("decrypting file %s by trying all possible keys %s...\n", argv[1]);
 // printf("To quit, press ctrl + c\n\n");
 // printf("Status:\n");




  if ((fin = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "ERROR: Could not open: %s\n", argv[1]);
    exit(1);
  }


	// Broadcasting from root process to all other processers
  encrypted[0] = 0;
  len = 0;
	MPI_Barrier(MPI_COMM_WORLD);

	if (my_rank == 0)
	{

  fseek(fin, 0, SEEK_END);
  len = ftell(fin);
  fseek(fin, 0, SEEK_SET);
  fread(encrypted, len, 1, fin);
  printf("[LOG] %u encrypted bytes read.\n", len); 
  fclose(fin);
	}


	tstart = MPI_Wtime();
	MPI_Bcast(encrypted, 1000, MPI_CHAR, 0, MPI_COMM_WORLD);








		  //printf("encrypted: ");
		//for (i=0; i<1000; ++i)
		  //  printf("[%d]: %c", i, encrypted[i]);




			// parallel code here
		 //	unsigned char l_decrypted[1000], l_dcopy[1000];


			//
			//test
			int data;

			// calls merges
			MPI_Request request[comm_sz];
			MPI_Irecv(&data,1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request[my_rank] );
			int flag = 0;
			int j = 0;
			int check_amount = 1000;
			unsigned long long iteration_end= pow(2,sizeof(int)*8);
			unsigned long long  block_sz = iteration_end / comm_sz;
			unsigned long long  start = block_sz * my_rank;
			int numberThreads = 28;

		#pragma omp parallel for num_threads(numberThreads)  private(i) shared(block_sz, start,check_amount)
		  for ( i = start  ; i <  (block_sz + start) ; ++i) {

				if (i %check_amount )
				{
						MPI_Test(&request[my_rank],&flag ,MPI_STATUS_IGNORE);
						if(flag)
						{
						//		printf("done : %d\n", my_rank);
								MPI_Finalize();
								exit(0);

						}
				}
         decrypt32(encrypted, i, decrypted, len);
//    printf("i=%d - decrypted: %s\n", i, decrypted); 

      memcpy(dcopy, decrypted, len * sizeof(unsigned char));
		    if (isValid(dcopy, len)) {
		      success = 1;
		//	#pragma omp parallel for num_threads(numberThreads) private(j)
			for (; j < comm_sz; ++j )
				MPI_Send(&success, 1, MPI_INT, j, 0, MPI_COMM_WORLD);
		      		break;
		    }

		  }



  //double final;
  tend = MPI_Wtime();
  //if (my_rank == 0) 
 //printf("\nTime elapsed: %.6f seconds\n", tend - tstart);
 printf("\nTime elapsed: %.6f seconds\n", tend- tstart); 

	MPI_Finalize();

  if (success) {   
    sprintf(outfilename, "%s.out", argv[1]);
    fout = fopen(outfilename, "w");
    fprintf(fout, "%s", decrypted);
    printf("\nFile decrypted successfully using key %llu\n", i);
    printf("See the file %s\n\n\n", outfilename);
    fclose(fout);
    return 0;
  }

  printf("\nWARNING: File could not be decrypted.\n\n\n");
  return 1;
}
