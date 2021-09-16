#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <mpi.h>
#include <chrono>
#include <omp.h>
#include <iomanip>

using namespace std::chrono;
using namespace std;

#define HEAD 0
#define THREADS 4

//Declare size and matrices outside of main so they can be accessed anywhere
int size = 100;
int **M1, **M2, **M3;

//Declare each of the functions before use
void init(int** &matrix, int rows, int cols, bool initialise);
void print( int** matrix);
void multiply(int** input1, int** input2, int** result, int num_rows);
void head(int num_processes);
void node(int process_rank, int num_processes);

int main(int argc, char** argv) {

    int num_processes, process_rank;

    //Set the humber of threads being used for omp
    omp_set_num_threads(THREADS);

    //Setup the MPI environment
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    //The head and the nodes have different functions
    if(process_rank == HEAD) 
        head(num_processes);
    else
        node(process_rank, num_processes);
    
    //Close out the MPI environment
    MPI_Finalize();
}

void head(int num_processes)
{
    //Initiliaze and allocate memory for matrices
    init(M1, size, size, true), init(M2, size, size, true), init(M3, size, size, false);

    //Calculated sizes for num of rows per processer, size of M2 to broadcast and size to scatter
    int num_rows = size / num_processes;
    int broadcast_size = (size * size);
    int scatter_gather_size = (size * size) / num_processes;

    //Start the clock prior to scattering data, taking into account this time
    auto start = high_resolution_clock::now();

    //Scatter the data in M1 across all processes evenly, broadcast the whole of M2 to all processes
    MPI_Scatter(&M1[0][0], scatter_gather_size ,  MPI_INT , &M1 , 0, MPI_INT, HEAD , MPI_COMM_WORLD);
    MPI_Bcast(&M2[0][0], broadcast_size , MPI_INT , HEAD , MPI_COMM_WORLD);
    
    //Run multiply on the first n(num_rows) rows in parallel with each other using omp
    #pragma omp parallel default(none) shared(M1, M2, M3, num_rows)
    {
        multiply(M1, M2, M3, num_rows);
    }

    //Wait here and gather all nodes M3 data into the main
    MPI_Gather(MPI_IN_PLACE, scatter_gather_size , MPI_INT, &M3[0][0] , scatter_gather_size , MPI_INT, HEAD , MPI_COMM_WORLD);

    //Stop the clock and print the result
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    cout << "Time taken by function: " << fixed << setprecision(3) << duration.count()/1000.0 << " seconds" << endl;
}

void node(int process_rank, int num_processes)
{
    //Calculated sizes for num of rows per processer, size of M2 to broadcast and size to scatter, done on each node as the values arnt global
    int num_rows = size / num_processes;
    int broadcast_size = (size * size);
    int scatter_gather_size = (size * size) / num_processes;

    //Initiliaize empty matrices for M1, M2 and M3, are filled when they are recieved in broadcast
    init(M1, num_rows, size, true), init(M2, size, size, false), init(M3, num_rows, size, false);

    //Receive M1 and M2 from the head broadcasting/scattering them out
    MPI_Scatter(NULL, scatter_gather_size , MPI_INT , &M1[0][0], scatter_gather_size, MPI_INT, HEAD , MPI_COMM_WORLD);
    MPI_Bcast(&M2[0][0], broadcast_size , MPI_INT , HEAD , MPI_COMM_WORLD);
    
    //Run multiply on the given matrices, (Will run though the whole of current M1 as it is already partitioned)
    //Runs in parallel using omp 
    #pragma omp parallel default(none) shared(M1, M2, M3, num_rows)
    {
        multiply(M1, M2, M3, num_rows);
    }

    //Send the calculated matrix back to the main/head
    MPI_Gather(&M3[0][0], scatter_gather_size , MPI_INT, NULL, scatter_gather_size , MPI_INT, HEAD , MPI_COMM_WORLD);
}

//Function for creating and initializing a matrix, using a pointer to pointers
void init(int** &matrix, int rows, int cols, bool initialise) 
{
    matrix = (int **) malloc(sizeof(int*) * rows * cols);  
    int* temp = (int *) malloc(sizeof(int) * cols * rows); 

    for(int i = 0 ; i < size ; i++) {
        matrix[i] = &temp[i * cols];
    }
  
    if(!initialise) return;

    for(long i = 0 ; i < rows; i++) {
        for(long j = 0 ; j < cols; j++) {
            matrix[i][j] = rand() % 100; 
        }
    }
}

//Function that prints out a matrix. If the matrices size is greater than 15 it only prints the
//first and last 5 items of the first and last 5 rows
void print( int** matrix) 
{
  if (size > 15)
    {
        for(long i = 0; i < 5; i++)
        {
            for (long j = 0; j < 5; j++)
            {                       
                printf("%d ",  matrix[i][j]); 
            }
            printf(" ..... ");
            for (long j = size - 5; j < size; j++)
            {                       
                printf("%d ",  matrix[i][j]); 
            }
            printf("\n");
        }
        printf(".\n");
        printf(".\n");
        printf(".\n");
        printf(".\n");
        printf(".\n");
        for(long i = size - 5; i < size; i++)
        {
            for (long j = 0; j < 5; j++)
            {                       
                printf("%d ",  matrix[i][j]); 
            }
            printf(" ..... ");
            for (long j = size - 5; j < size; j++)
            {                       
                printf("%d ",  matrix[i][j]); 
            }
            printf("\n");
        }
    }
    else
    {
        for (long i = 0; i < size; i++)
        {                       
            for(long j = 0 ; j < size; j++) {  
                printf("%d ",  matrix[i][j]); 
            }
            printf("\n"); 
        }
    }

    printf("\n----------------------------\n");
}

//Matrix multiplication function the multiply two given matrices, with the first n(num_rows) of the first matrix
void multiply(int** input1, int** input2, int** result, int num_rows)
{
    //multiply the relevant rows of the first matrix with the second matrix
    #pragma omp for
    for(int i = 0; i < num_rows ; i++) {
        for(int j = 0; j < size; j++) {
            //Add up the multiplications of the rows elements with each columns elements
            for(int k = 0; k < size; k++)
                result[i][j] = result[i][j] + (input1[i][k] * input2[k][j]);
        }
    }
}
