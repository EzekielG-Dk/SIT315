#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <chrono>


using namespace std::chrono;
using namespace std;

//Structs to store vector reference and endpoints for each threads iteration
struct arg_struct {
    int* vector;
    int start;
    int end;
};

struct add_struct {
    int* v1;
    int* v2;
    int* v3;
    int start;
    int end;
};

void* randomVector(void* arguments)
{
    struct arg_struct* args = (struct arg_struct*)arguments;
    for (int i = args->start; i < args->end; i++)
    {
        //Create random array elements using modulus 100 to get remainders as data
        args->vector[i] = rand() % 100;
    }
    return (void*)args->vector;
}

//Add vector function to enable multithreading of addition
void* addVector(void* arguments)
{
    struct add_struct* args = (struct add_struct*)arguments;
    //Add elements of both v1 and v2 together into v3
    for (int i = args->start; i < args->end; i++)
    {
        args->v3[i] = args->v1[i] + args->v2[i];
    }

    return (void*)args->v3;
}

//First attempt at multithreading currently this code uses for threads for both the random filling of the vectors and 
//the addition of the two vectors together. The code and useability of it can be optimised though by creating loops for
//the creation of the structs and different threads and placing their references in lists. By doing this we could have a
//input so that the loops can vary in size and therefore vary in the number of threads used.
int main() {

    unsigned long size = 100000000;

    srand(time(0));

    int* v1, * v2, * v3;

    //Start clock to calculate runtime of program
    auto start = high_resolution_clock::now();

    //Create vector arrays and allocate to memory
    v1 = (int*)malloc(size * sizeof(int*));
    v2 = (int*)malloc(size * sizeof(int*));
    v3 = (int*)malloc(size * sizeof(int*));

    //Create a struct for each thread to hand to p_thread and randomVector function
    //Each struct contains a reference to the vector and the start and end points
    struct arg_struct* struct1_1 = (struct arg_struct*)malloc(sizeof(struct arg_struct));
    struct1_1->vector = v1;
    struct1_1->start = 0;
    struct1_1->end = size / 2;

    struct arg_struct* struct1_2 = (struct arg_struct*)malloc(sizeof(struct arg_struct));
    struct1_2->vector = v1;
    struct1_2->start = size / 2 + 1;
    struct1_2->end = size;

    struct arg_struct* struct2_1 = (struct arg_struct*)malloc(sizeof(struct arg_struct));
    struct2_1->vector = v2;
    struct2_1->start = 0;
    struct2_1->end = size / 2;

    struct arg_struct* struct2_2 = (struct arg_struct*)malloc(sizeof(struct arg_struct));
    struct2_2->vector = v2;
    struct2_2->start = size / 2 + 1;
    struct2_2->end = size;

    //Initialize the thread variables
    pthread_t vector1_1;
    pthread_t vector1_2;
    pthread_t vector2_1;
    pthread_t vector2_2;

    //Call the function on each thread using the previously created structs as input
    pthread_create(&vector1_1, NULL, randomVector, (void*)struct1_1);
    pthread_create(&vector1_2, NULL, randomVector, (void*)struct1_2);
    pthread_create(&vector2_1, NULL, randomVector, (void*)struct2_1);
    pthread_create(&vector2_2, NULL, randomVector, (void*)struct2_2);

    //Join the threads back together
    void* v1_1;
    pthread_join(vector1_1, &v1_1);
    void* v1_2;
    pthread_join(vector1_2, &v1_2);
    void* v2_1;
    pthread_join(vector2_1, &v2_1);
    void* v2_2;
    pthread_join(vector2_2, &v2_2);

    //Create a struct for each thread to hand to p_thread and addVector function
    //Each struct contains a reference to v1, v2 and v3 as well as the start and endpoints
    //for each thread
    struct add_struct* addstruct1 = (struct add_struct*)malloc(sizeof(struct add_struct));
    addstruct1->v1 = v1;
    addstruct1->v2 = v2;
    addstruct1->v3 = v3;
    addstruct1->start = 0;
    addstruct1->end = size / 4;

    struct add_struct* addstruct2 = (struct add_struct*)malloc(sizeof(struct add_struct));
    addstruct2->v1 = v1;
    addstruct2->v2 = v2;
    addstruct2->v3 = v3;
    addstruct2->start = size / 4 + 1;
    addstruct2->end = size / 2;

    struct add_struct* addstruct3 = (struct add_struct*)malloc(sizeof(struct add_struct));
    addstruct3->v1 = v1;
    addstruct3->v2 = v2;
    addstruct3->v3 = v3;
    addstruct3->start = size / 2 + 1;
    addstruct3->end = size / 4 * 3;

    struct add_struct* addstruct4 = (struct add_struct*)malloc(sizeof(struct add_struct));
    addstruct4->v1 = v1;
    addstruct4->v2 = v2;
    addstruct4->v3 = v3;
    addstruct4->start = size / 4 * 3 + 1;
    addstruct4->end = size;

    //Initialize the thread variables
    pthread_t add1;
    pthread_t add2;
    pthread_t add3;
    pthread_t add4;

    //Call the addfunction on each thread using the previously created structs
    pthread_create(&add1, NULL, addVector, (void*)addstruct1);
    pthread_create(&add2, NULL, addVector, (void*)addstruct2);
    pthread_create(&add3, NULL, addVector, (void*)addstruct3);
    pthread_create(&add4, NULL, addVector, (void*)addstruct4);

    //Join the threads back together
    void* added1;
    pthread_join(add1, &added1);
    void* added2;
    pthread_join(add2, &added2);
    void* added3;
    pthread_join(add3, &added3);
    void* added4;
    pthread_join(add4, &added4);

    auto stop = high_resolution_clock::now();

    //Calculate duration based on clock start and stop time
    auto duration = duration_cast<microseconds>(stop - start);


    cout << "Time taken by function: "
        << duration.count() << " microseconds" << endl;

    return 0;
}
