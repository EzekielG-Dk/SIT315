#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <omp.h>
#include <fstream>

using namespace std::chrono;
using namespace std;

#define size 50
#define THREADS 8

//Function that randomizes the elements within a matrix
void randomMatrix(int *matrix[])
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            matrix[i][j] = rand() % 100;
        }
    }
}

//Function that multiplies two matrices into a third
void multiplyMatrix(int *matrix1[], int *matrix2[], int *matrix3[])
{
    //Parallelize the first for loop of the multiplication as this will split each of the rows
    //into sections and then each uses every column of the scond graph
    #pragma omp for
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            int sum = 0;
            for (int k = 0; k < size; k++)
                sum = sum + (matrix1[i][k] * matrix2[k][j]);
            matrix3[i][j] = sum;
        }
    }
}

int main() {

    srand(time(0));

    //Create and allocate memory for each matrix using an array of arrays, allocating memory to each one
    int** m1 = new int* [size];
    for (int i = 0; i < size; i++) 
        m1[i] = new int[size];

    int** m2 = new int* [size];
    for (int i = 0; i < size; i++) 
        m2[i] = new int[size];

    int** m3 = new int* [size];
    for (int i = 0; i < size; i++) 
        m3[i] = new int[size];

    //Set the omp number of threads to whatever the global is
    omp_set_num_threads(THREADS);

    //Randomize the matrices in non-parallel
    randomMatrix(m1);
    randomMatrix(m2);

    auto start = high_resolution_clock::now();

    //Run the multiply matrix function in parallel
    #pragma omp parallel default(none) shared(m1, m2, m3)
    {
        multiplyMatrix(m1, m2, m3);
    }

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop - start);

    ofstream myfile;
    myfile.open("OpenMP.txt", fstream::app);

    myfile << "Time to multiply two " << size << "X" << size << " matrices with OpenMP using "
         << THREADS << " threads: " << duration.count() << " microseconds" << endl;
     
    myfile.close();

    cout << "Time taken by function: "
        << duration.count() << " microseconds" << endl;
}