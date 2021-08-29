#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <omp.h>


using namespace std::chrono;
using namespace std;

#define size 500000
#define THREADS 2

//Swap function takes in  two items and switchs them
void swap(int* x, int* y)
{
    cout << "X: " << x;
    cout << "Y: " << y;
    int temp = *x;
    *x = *y;
    *y = temp;
}

//Partiion function that takes a min and max and puts all les than max to the left of it and all items
//greater than it to the right of it
int partition(int *array, int min, int max)
{

    int pivot = array[max];
    int left = min - 1;

    for (int i = min; i <= max - 1; i++)
    {
        if (array[i] <= pivot)
        {
            left++;
            swap(array[left], array[i]);
        }
    }
    swap(array[left + 1], array[max]);

    //Returns the center point of this pivot, so that recursive quicksort calls are run on each side of it
    return (left + 1);
}

//randPartition function run by the quicksort algorithm that takes a random item from the array and swaps it to
//the end to be partitioned. This is done because a normal quicksort has an expected has runtime of O(n^2)
//whilst randomized has an expected runtime of O(nlog(n)), much faster
int randPartition(int *array, int min, int max)
{
    int random = min + rand() % (max - min);

    swap(array[random], array[max]);

    return partition(array, min, max);
}

//Recursive quick sort function takes and array and two endpoints, this is recursive so can be the each end of the
//array or just a small section within it
void quickSort(int *array, int min, int max)
{
    //If the provided min is still lower than the provided max perform the quicksort. This is because at the tips of
    //each branch we will reach a point where we only have one value
    if (min < max)
    {
        //Partition the array with everything lower than max on left of split index and everything greater on the right
        int split = randPartition(array, min, max);

        //Run each of the quicksort call as a seperate parallel task, recursively down until max threads. OpenMP decides itself
        #pragma omp task shared(array)
        quickSort(array, min, split - 1);
        #pragma omp task shared(array)
        quickSort(array, split + 1, max);
    }
}

//Function that prints an array
void printArray(int *array[])
{
    for (int i = 0; i < size; i++)
        printf("%d ", array[i]);
    printf("\n");
}

int main() {

    srand(time(0));

    //Initialize the array, allocate space in memory
    int* array = (int*)malloc(size * sizeof(int*));

    for (int i = 0; i < size; i++)
    {
        array[i] = rand() % 100;
    }

    omp_set_num_threads(THREADS);

    auto start = high_resolution_clock::now();

    //Start the parallel part of the code
    #pragma omp parallel
    {
        //The initial quicksort call needs to be run on only a single thread not duplication or parallization
        #pragma omp single
        quickSort(array, 0, size - 1);
    }
    
    auto stop = high_resolution_clock::now();

    auto duration = (duration_cast<microseconds>(stop - start));
    double duration2 = duration.count()/1000000.0;

    cout << "Time taken by function: "
         << duration2 << " seconds" << endl;
}
