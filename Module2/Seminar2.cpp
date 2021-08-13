#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <chrono>


using namespace std::chrono;
using namespace std;

//Structs to store vector reference and endpoints for each threads iteration
//To be provided to the functions that are run in parallel
struct find_struct {
    int *vector;
    int start;
    int end;
};

struct add_struct {
    int *v1, *v2, *v3;
    int start;
    int end;
};

void* randomVector(void* arguments)
{
    find_struct *args = (struct find_struct *)arguments;
    for (int i = args->start; i < args->end; i++)
    {
        //Create random array elements using modulus 100 to get remainders as data
        args->vector[i] = rand() % 100;
    }
    return NULL;
}

//Add vector function to enable multithreading of addition
void* addVector(void* arguments)
{
    add_struct *args = (struct add_struct *)arguments;
    //Add elements of both v1 and v2 together into v3
    for (int i = args->start; i < args->end; i++)
    {
        args->v3[i] = args->v1[i] + args->v2[i];
    }

    return NULL;
}

int main() {

    unsigned long size = 100000000L;

    srand(time(0));

    int* v1, * v2, * v3;
    const int threads = 8;

    //Start clock to calculate runtime of program
    auto start = high_resolution_clock::now();

    //Create vector arrays and allocate to memory
    v1 = (int*)malloc(size * sizeof(int*));
    v2 = (int*)malloc(size * sizeof(int*));
    v3 = (int*)malloc(size * sizeof(int*));

    //Create a list of threads to divy out
    pthread_t active_find[threads];

    int partition = size/(threads/2);

    //Partition half of the threads to work on filling the first vector with random values
    for (int i = 0; i < threads/2; i++)
    {
        struct find_struct *task = (struct find_struct *)malloc(sizeof(struct find_struct));
        task->vector = v1;
        task->start = i * partition;
        task->end = (i + 1) == (threads) ? size : ((i + 1) * partition);
        pthread_create(&active_find[i], NULL, randomVector, (void*)task);
    }
    //Partition the second half of the threads to work on filling the second vector with random values
    for (int i = 0; i < threads/2; i++)
    {
        struct find_struct *task = (struct find_struct *)malloc(sizeof(struct find_struct));
        task->vector = v2;
        task->start = i * partition;
        task->end = (i + 1) == (threads) ? size : ((i + 1) * partition);
        pthread_create(&active_find[i], NULL, randomVector, (void*)task);
    }

    //Wait for all the threads to join back with the main thread
    for (int i = 0; i < threads; i++)
    {
        pthread_join(active_find[i], NULL);
    }

    //Create a list of threads to divy out
    pthread_t active_add[threads];

    //Partition out the threads to each work on adding different parts of the two vectors together
    for (int i = 0; i < threads; i++)
    {
        struct add_struct *task = (struct add_struct *)malloc(sizeof(struct add_struct));
        task->v1 = v1;
        task->v2 = v2;
        task->v3 = v3;
        task->start = i*partition;
        task->end = (i + 1) == threads ? size : ((i + 1) * partition);
        pthread_create(&active_add[i], NULL, addVector, (void*)task);
    }

    //Wait for all the threads to join back with the main thread
    for (int i = 0; i < threads; i++)
    {
        pthread_join(active_add[i], NULL);
    }

    auto stop = high_resolution_clock::now();
    //Calculate duration based on clock start and stop time
    auto duration = duration_cast<microseconds>(stop - start);

    cout << "Time taken by function: "
        << duration.count() << " microseconds" << endl;

    return 0;
}
