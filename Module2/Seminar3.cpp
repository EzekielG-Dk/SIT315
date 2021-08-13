#include <omp.h>

#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>


using namespace std::chrono;
using namespace std;

int main() {

    unsigned long size = 10000000;

    srand(time(0));

    int* v1, * v2, * v3;

    //Set the number of threads and default values for the total variables
    int THREADS_NO = 8;
    int total_redu = 0;
    int total_crit = 0;
    int total_atom = 0;
    int no_total = 0;

    omp_set_num_threads(THREADS_NO);

    auto start = high_resolution_clock::now();

    v1 = (int*)malloc(size * sizeof(int*));
    v2 = (int*)malloc(size * sizeof(int*));
    v3 = (int*)malloc(size * sizeof(int*));

    //A omp parallel function
    #pragma omp parallel default(none) shared(v1, v2, size, v3)
    {
        //Fill arrays with data via parallel
        #pragma omp for schedule(dynamic, 4)
        for (int i = 0; i < size; i++)
        {
            v1[i] = rand() % 100;
        }
        #pragma omp for schedule(dynamic, 4)
        for (int i = 0; i < size; i++)
        {
            v2[i] = rand() % 100;
        }

        //Add arrays together using omp parallel for
        #pragma omp for schedule(dynamic, 4)
        for (int i = 0; i < size; i++)
        {
            v3[i] = v1[i] + v2[i];
        }
    }

    auto start1 = high_resolution_clock::now();

    //Calculate total using atomic attributes
     #pragma omp parallel default(none) shared(v3, total_atom, size)
    {
        #pragma omp for
        for (int i = 0; i < size; i++)
        {
            #pragma omp atomic
            total_atom += v3[i];
        }
    }

    auto stop1 = high_resolution_clock::now();
    auto start2 = high_resolution_clock::now();
    
    //Calculate total using a reduction
    #pragma omp parallel default(none) shared(v3, size) reduction(+:total_redu)
    {
        #pragma omp for
        for (int i = 0; i < size; i++)
        {
            total_redu += v3[i];
        }
    }

    auto stop2 = high_resolution_clock::now();
    auto start3 = high_resolution_clock::now();

    //Calculate total using a seperate private variable for each thread added to another one
    #pragma omp parallel default(none) shared(total_crit, v3, size) firstprivate(no_total)
    {
        #pragma omp for
        for (int i = 0; i < size; i++)
        {
            no_total += v3[i];
        }

        #pragma omp critical
        {
            total_crit += no_total;
        }
    }

    auto stop3 = high_resolution_clock::now();
    auto stop = high_resolution_clock::now();

    auto duration = duration_cast<microseconds>(stop - start);
    auto duration1 = duration_cast<microseconds>(stop1 - start1);
    auto duration2 = duration_cast<microseconds>(stop2 - start2);
    auto duration3 = duration_cast<microseconds>(stop3 - start3);

    cout << "Time taken by function: "
        << duration.count() << " microseconds" << endl;

    cout << "Total using atomic: " << total_atom << " in " << duration1.count() << " seconds" << endl;
    cout << "Total using reduction: " << total_redu << " in " << duration2.count() << " seconds" << endl;
    cout << "Total using critical: " << total_crit << " in " << duration3.count() << " seconds" << endl;

    return 0;
}
