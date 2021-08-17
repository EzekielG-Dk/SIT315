#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <pthread.h>
#include <fstream>

using namespace std::chrono;
using namespace std;

#define size 50
#define THREADS 8

int** m1 = new int* [size];
int** m2 = new int* [size];
int** m3 = new int* [size];

int randThread1 = 0;
int randThread2 = 0;
int multiThread = 0;

//The randomization of values in the two matrices use different functions. Where each thread working on
//the same martix uses the same function. This is because the functions draw from global variables to get
//to perform their task. The functions all also have and unused variable with NULL so that pthread can use it
void* randomMatrix1(void *unused)
{
    //Gets the current thread for the first matrix and what the partition size is
    int thread = randThread1++;
    int partition = size / (THREADS/2);

    //For each element in the matrix in the space of this partition give it a random value
    for (int i = thread * partition; i < (thread + 1) * partition; i++)
    {
        for (int j = 0; j < size; j++)
        {
            m1[i][j] = rand() % 100;
        }
    }

    return NULL;
}

//Everything here is the same as the previous function except working with the second matrix
void* randomMatrix2(void *unused)
{
    int thread = randThread2++;
    int partition = size / (THREADS/2);

    for (int i = thread * partition; i < (thread + 1) * partition; i++)
    {
        for (int j = 0; j < size; j++)
        {
            m2[i][j] = rand() % 100;
        }
    }

    return NULL;
}

//Similar to that of previous function in which the matrix and current thread are drawn from global 
//variables but instead and third iteration takes place where the two matrices are multiplied
void* multiplyMatrix(void* unused)
{
    int thread = multiThread++;
    int partition = size / THREADS;

    for (int i = thread * partition; i < (thread + 1) * partition; i++)
    {
        for (int j = 0; j < size; j++)
        {
            int sum = 0;
            for (int k = 0; k < size; k++)
            {
                sum = sum + (m1[i][k] * m2[k][j]);
            }
            m3[i][j] = sum;
        }
    }

    return NULL;
}

int main() {

    srand(time(0));

    //Fill allocate memory for the arrays within the global variable arrays
    for (int i = 0; i < size; i++) 
    m1[i] = new int[size];

    for (int i = 0; i < size; i++) 
    m2[i] = new int[size];

    for (int i = 0; i < size; i++) 
    m3[i] = new int[size];

    //Array of threads for randomization
    pthread_t randThre[THREADS];

    //Tested both a multithreaded and sequential calculation of this and determined
    //the multithreaded was faster enough on larger data sizes that it was good to
    //use
    for (int i = 0; i < THREADS / 2; i++)
    {
        pthread_create(&randThre[i], NULL, randomMatrix1, NULL);
    }
    for (int i = 0; i < THREADS / 2; i++)
    {
        pthread_create(&randThre[i + THREADS / 2], NULL, randomMatrix2, NULL);
    }
    //Wait until all the threads are joined back with the main thread
    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(randThre[i], NULL);
    }

    //Array of threads for multiplcation
    pthread_t multThre[THREADS];

    //Clock to track runtime
    auto start = high_resolution_clock::now();

    for (int i = 0; i < THREADS; i++)
    {
        pthread_create(&multThre[i], NULL, multiplyMatrix, NULL);
    }
    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(multThre[i], NULL);
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    ofstream myfile;
    myfile.open("Pthread.txt", fstream::app);

    myfile << "Time to multiply two " << size << "X" << size << " matrices with Pthread using "
         << THREADS << " threads: " << duration.count() << " microseconds" << endl;
     
    myfile.close();

    cout << "Time taken by function: "
        << duration.count() << " microseconds" << endl;
}