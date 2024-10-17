#include <iostream>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>

#include "mpi.h"
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

using namespace std;

#define MASTER 0

void printArray(int* arr, int n) {
    for (int i = 0; i < n; ++i) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

int main(int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;

    // Initializing MPI
    MPI_Init(&argc, &argv);

    // Get number of processes and current process rank
    int num_processes, rank, n;
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    n = stoi(argv[1]);
    string input_type = argv[2];

    // Create a new local array based on the number of processors
    int localSize = n / num_processes;
    int* localArray = new int[localSize];

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> totalDis(0, n/4);
    uniform_int_distribution<int> localDis(0, localSize - 1);

    cali::ConfigManager mgr;
    mgr.start();

    CALI_MARK_BEGIN("data_init_runtime");
    if (input_type == "Random") {
        for (int i = 0; i < localSize; i++) {
            localArray[i] = totalDis(gen);
        }
    } else if (input_type == "Sorted") {
        for (int i = 0; i < localSize; i++) {
            localArray[i] = i + rank * localSize;
        }
    } else if (input_type == "ReverseSorted") {
        for (int i = 0; i < localSize; i++) {
            localArray[i] = n - i - 1 - rank * localSize;
        }
    } else if (input_type == "1_perc_perturbed") {
        for (int i = 0; i < localSize; i++) {
            localArray[i] = i + rank * localSize;
        }

        for (int i = 0; i < localSize; i++) {
            if (localDis(gen) == 0) { // approximately 1% chance
                localArray[i] = totalDis(gen) * 1000;
            }
        }
    }
    CALI_MARK_END("data_init_runtime");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    sort(localArray, localArray + localSize);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    int num_stages = log2(num_processes);

    for (int stage = 1; stage <= num_stages; stage++) {
        for (int step = stage; step > 0; step--) {

            CALI_MARK_BEGIN("comp");
            CALI_MARK_BEGIN("comp_small");
            int partner_rank = rank ^ (1 << (step - 1));

            // Calculate the group number and the direction of the sort
            int group_size = 1 << stage;
            int group = rank / group_size;
            bool ascending = (group % 2 == 0);

            int* partner_arr = new int[localSize];
            CALI_MARK_END("comp_small");
            CALI_MARK_END("comp");

            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large");
            MPI_Sendrecv(localArray, localSize, MPI_INT, partner_rank, 0, partner_arr, localSize, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");

            CALI_MARK_BEGIN("comp");
            CALI_MARK_BEGIN("comp_large");
            // Merge the local array with the partner array (while maintaining sorted order)
            int* merged = new int[localSize * 2];
            int i = 0, j = 0, k = 0;

            while (i < localSize && j < localSize) {
                if (localArray[i] <= partner_arr[j]) {
                    merged[k++] = localArray[i++];
                } else {
                    merged[k++] = partner_arr[j++];
                }
            }

            while (i < localSize) {
                merged[k++] = localArray[i++];
            }

            while (j < localSize) {
                merged[k++] = partner_arr[j++];
            }
            
            // Copy the merged array back to the local array based on the direction of the sort
            if (rank < partner_rank && ascending || rank > partner_rank && !ascending) {
                for (int i = 0; i < localSize; i++) {
                    localArray[i] = merged[i];
                }
            } else {
                for (int i = 0; i < localSize; i++) {
                    localArray[i] = merged[i + localSize];
                }
            }

            delete[] partner_arr;
            delete[] merged;
            CALI_MARK_END("comp_large");
            CALI_MARK_END("comp");
        }
        CALI_MARK_BEGIN("comm");
        MPI_Barrier(MPI_COMM_WORLD);
        CALI_MARK_END("comm");
    }

    CALI_MARK_BEGIN("correctness_check");
    // Correctness check
    // Checking if our local array sorted correctly
    bool localSorted = true;
    for (int i = 1; i < localSize; ++i) {
        if (localArray[i] < localArray[i-1]) {
            localSorted = false;
            break;
        }
    }

    // Checking to see that the minimum element in the next rank is >= the local maximum element
    int localMax = localArray[localSize - 1];
    int nextMin = 0;
    bool boundaryCheck = true;

    if (rank < num_processes - 1) {
        MPI_Send(&localMax, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    }

    if (rank != 0) {
        MPI_Recv(&nextMin, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        boundaryCheck = (nextMin <= localArray[0]);
    }

    bool isSorted = localSorted && boundaryCheck;

    int globalSorted;
    MPI_Allreduce(&isSorted, &globalSorted, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
    CALI_MARK_END("correctness_check");

    if (rank == MASTER) {
        if (globalSorted) {
            cout << "Bitonic sort PASSED" << endl;
        } else {
            cout << "Bitonic sort FAILED" << endl;
        }
    }

    delete[] localArray;

    if (rank == MASTER) {
        adiak::init(NULL);
        adiak::launchdate();    // launch date of the job
        adiak::libraries();     // Libraries used
        adiak::cmdline();       // Command line used to launch the job
        adiak::clustername();   // Name of the cluster
        adiak::value("algorithm", "bitonic"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
        adiak::value("programming_model", "mpi"); // e.g. "mpi"
        adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
        adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
        adiak::value("input_size", n); // The number of elements in input dataset (1000)
        adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
        adiak::value("num_procs", num_processes); // The number of processors (MPI ranks)
        adiak::value("scalability", "weak"); // The scalability of your algorithm. choices: ("strong", "weak")
        adiak::value("group_num", "1"); // The number of your group (integer, e.g., 1, 10)
        adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
    }

    // Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();

    MPI_Finalize();
    return 0;
}