#include <iostream>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>

#include <limits.h>
#include <vector>
#include <ctime>

#include "mpi.h"
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

using namespace std;

#define MASTER 0

std::vector<int> random(size_t size, size_t total_size, int rank) {
    std::mt19937 gen(std::time(0) + rank * 1000);
    std::uniform_int_distribution<> dist(0, total_size);
    std::vector<int> vec(size);
    std::generate(vec.begin(), vec.end(), [&]() { return dist(gen); });
    return vec;
}

std::vector<int> sorted(size_t size, size_t total_size, int rank) {
    std::vector<int> vec;
    for (size_t i = rank * size; i < size * (rank + 1); i++) {
        vec.push_back(i);
    }
    return vec;
}

std::vector<int> reversed(size_t size, size_t total_size, int rank) {
    std::vector<int> vec;
    for (size_t i = total_size - (size * rank); i > total_size - size * (rank + 1); i--) {
        vec.push_back(i);
    }
    return vec;
}

std::vector<int> perturbed(size_t size, size_t total_size, int rank) {
    std::vector<int> vec = sorted(size, total_size, rank);
    std::mt19937 gen(std::time(0) + rank * 1000);
    std::uniform_int_distribution<> dist(0, total_size);
    std::uniform_int_distribution<> perturb(0, 99);
    for (size_t i = 0; i < size; i++) {
        if (perturb(gen) == 0) {
            vec[i] = dist(gen);
        }
    }
    return vec;
}

bool is_globally_sorted(const std::vector<int>& local_data, int rank, int num_procs) {
    bool local_sorted = std::is_sorted(local_data.begin(), local_data.end());

    int local_first, local_last;
    if (local_data.empty()) {
        local_first = INT_MAX;
        local_last = INT_MIN;
    } else {
        local_first = local_data.front();
        local_last = local_data.back();
    }

    int prev_last = INT_MIN;
    if (rank > 0) {
        MPI_Recv(&prev_last, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    if (rank < num_procs - 1) {
        MPI_Send(&local_last, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    }

    bool is_in_order = true;
    if (rank > 0 && local_first < prev_last) {
        is_in_order = false;
    }

    bool globally_sorted = local_sorted && is_in_order;
    bool final_sorted;
    MPI_Allreduce(&globally_sorted, &final_sorted, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);

    return final_sorted;
}


int main(int argc, char** argv) {
    CALI_CXX_MARK_FUNCTION;

    // Initializing MPI
    MPI_Init(&argc, &argv);

    // Get number of processes and current process rank
    int num_procs, rank, totalSize;
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cali::ConfigManager mgr;
    mgr.start();

    totalSize = stoi(argv[1]);
    string input_type = argv[2];

    CALI_MARK_BEGIN("data_init_runtime");
    // Create a new local array based on the number of processors
    int localSize = totalSize / num_procs;
    vector<int> localArray;
    if (input_type == "Random") {
        localArray = random(localSize, totalSize, rank);
    } else if (input_type == "Sorted") {
        localArray = sorted(localSize, totalSize, rank);
    } else if (input_type == "ReverseSorted") {
        localArray = reversed(localSize, totalSize, rank);
    } else if (input_type == "1_perc_perturbed") {
        localArray = perturbed(localSize, totalSize, rank);
    }

    CALI_MARK_END("data_init_runtime");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    sort(localArray.begin(), localArray.end());
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    int num_stages = log2(num_procs);

    for (int stage = 1; stage <= num_stages; stage++) {
        for (int step = stage; step > 0; step--) {

            CALI_MARK_BEGIN("comp");
            CALI_MARK_BEGIN("comp_small");
            int partner_rank = rank ^ (1 << (step - 1));

            // Calculate the group number and the direction of the sort
            int group_size = 1 << stage;
            int group = rank / group_size;
            bool ascending = (group % 2 == 0);

            vector<int> partner_arr(localSize);
            CALI_MARK_END("comp_small");
            CALI_MARK_END("comp");

            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_large");
            MPI_Sendrecv(localArray.data(), localSize, MPI_INT, partner_rank, 0, partner_arr.data(), localSize, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");

            CALI_MARK_BEGIN("comp");
            CALI_MARK_BEGIN("comp_large");
            // Merge the local array with the partner array (while maintaining sorted order)
            vector<int> merged(localSize * 2);
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
            CALI_MARK_END("comp_large");
            CALI_MARK_END("comp");
        }
        CALI_MARK_BEGIN("comm");
        MPI_Barrier(MPI_COMM_WORLD);
        CALI_MARK_END("comm");
    }

    CALI_MARK_BEGIN("correctness_check");
    bool globally_sorted = is_globally_sorted(localArray, rank, num_procs);
    CALI_MARK_END("correctness_check");

    if (rank == 0) {
        if (globally_sorted) {
            printf("The data is globally sorted.\n");
        } else {
            printf("The data is NOT globally sorted.\n");
        }
    }

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
        adiak::value("input_size", totalSize); // The number of elements in input dataset (1000)
        adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
        adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
        adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
        adiak::value("group_num", "1"); // The number of your group (integer, e.g., 1, 10)
        adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
    }

    // Flush Caliper output before finalizing MPI
    mgr.stop();
    mgr.flush();

    MPI_Finalize();
    return 0;
}