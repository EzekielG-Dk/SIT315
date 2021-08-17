#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <fstream>


using namespace std::chrono;
using namespace std;

#define size 500

//Function that randomizes the elements within a matrix
void randomMatrix(int *matrix[])
{
    for (int i = 0; i < size; i++)
    {
        for(int j = 0; j < size; j++)
        {
            matrix[i][j] = rand() % 100;
        }
    }
}

//Function that multiplies two matrices into a third
void multiplyMatrix(int *matrix1[], int *matrix2[], int *matrix3[])
{
    //For each row
    for(int i = 0; i < size; i++)
    {
        //Go through each column
        for(int j = 0; j < size; j++)
        {
            //Add up the multiplications of the rows elements with each columns elements
            int sum = 0;
            for(int k = 0; k < size; k++)
                sum = sum + (matrix1[i][k] * matrix2[k][j]);
            matrix3[i][j] = sum;
        }
    }
}

int main(){

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

    //Randomize elements within the matrices
    randomMatrix(m1);
    randomMatrix(m2);


    auto start = high_resolution_clock::now();
    //Multiply the matrices together
    multiplyMatrix(m1, m2, m3);

    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop - start);

    ofstream myfile;
    myfile.open("Sequential.txt", fstream::app);

    myfile << "Time to multiply two " << size << "X" << size << " matrices sequentially using"
         << " 1" << " threads: " << duration.count() << " microseconds" << endl;
     
    myfile.close();

    cout << "Time taken by function: "
         << duration.count() << " microseconds" << endl;
}