#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

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

int main (int argc, char *argv[]) {
    CALI_CXX_MARK_FUNCTION;

    MPI_Init(&argc, &argv);

    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // Create caliper ConfigManager object
    cali::ConfigManager mgr;
    mgr.start();

    // Generate data to be sorted with the size and split evenly between processors
    // In Sample Sort, this is the first step
    // size_t total_size = pow(2, 22);
    // size_t total_size = 128;
    size_t total_size = std::stoi(argv[1]);
    std::string input_type = argv[2];

    // Adiak
    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "sample"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", total_size); // The number of elements in input dataset (1000)
    adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 1); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
    
    // DATA INITIALIZATION START
    CALI_MARK_BEGIN("data_init_runtime");
    
    // Won't need for inputs of this project but checks for uneven distribution
    size_t base_size = total_size / num_procs;
    // size_t remainder = total_size % num_procs;

    // size_t local_size;
    // if (rank < remainder) {
    //     local_size = base_size + 1;
    // } else {
    //     local_size = base_size;
    // }

    size_t local_size = base_size;

    // std::vector<int> data = random(local_size, total_size, rank);
    // std::vector<int> data = sorted(local_size, total_size, rank);
    // std::vector<int> data = reversed(local_size, total_size, rank);
    // std::vector<int> data = perturbed(local_size, total_size, rank);

    std::vector<int> data;
    if (input_type == "Random") {
        data = random(local_size, total_size, rank);
    } else if (input_type == "Sorted") {
        data = sorted(local_size, total_size, rank);
    } else if (input_type == "ReverseSorted") {
        data = reversed(local_size, total_size, rank);
    } else if (input_type == "1_perc_perturbed") {
        data = perturbed(local_size, total_size, rank);
    }

    CALI_MARK_END("data_init_runtime");
    // DATA INITIALIZATION END


    // Sort the data in each processor
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    std::sort(data.begin(), data.end());
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    // Sample selection
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    // Calculate the number of samples to be taken
    // Oversampling
    int k = 2;
    int num_samples = k * num_procs;

    // Get samples
    std::vector<int> local_samples;
    for (int i = 1; i <= num_samples; i++) {
        local_samples.push_back(data[((i * local_size) / (num_samples + 1)) % local_size]);
    }
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");


    // Gather all samples
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    std::vector<int> all_samples(num_samples * num_procs);
    MPI_Allgather(local_samples.data(), num_samples, MPI_INT, all_samples.data(), num_samples, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");
    
    // Sort samples
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    std::sort(all_samples.begin(), all_samples.end());
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    // Select pivots
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    std::vector<int> pivots(num_procs - 1);
    for (int i = 1; i < num_procs; i++) {
        size_t index = i * all_samples.size() / num_procs;
        if (index >= all_samples.size()) {
            index = all_samples.size() - 1;
        }
        pivots[i - 1] = all_samples[index];
    }
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    // Partition the data
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    std::vector<std::vector<int>> pivoted_buckets(num_procs);
    for (size_t i = 0; i < local_size; i++) {
        int bucket_index = 0;
        while (bucket_index < num_procs - 1 && data[i] > pivots[bucket_index]) {
            bucket_index++;
        }
        pivoted_buckets[bucket_index].push_back(data[i]);
    }
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    // Get send/receive data
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    std::vector<int> send_counts(num_procs), recv_counts(num_procs);
    for (int i = 0; i < num_procs; i++) {
        send_counts[i] = pivoted_buckets[i].size();
    }
    MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");

    // Prepare send/recv displacements for sending and receiving data
    std::vector<int> send_displs(num_procs), recv_displs(num_procs);
    int total_send = 0, total_recv = 0;
    for (int i = 0; i < num_procs; i++) {
        send_displs[i] = total_send;
        recv_displs[i] = total_recv;
        total_send += send_counts[i];
        total_recv += recv_counts[i];
    }

    // Prepare the data to be sent
    std::vector<int> send_data(total_send);
    int offset = 0;
    for (int i = 0; i < num_procs; i++) {
        std::copy(pivoted_buckets[i].begin(), pivoted_buckets[i].end(), send_data.begin() + offset);
        offset += send_counts[i];
    }

    // Prepare the data to be received and exchange data
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    std::vector<int> recv_data(total_recv);
    MPI_Alltoallv(send_data.data(), send_counts.data(), send_displs.data(), MPI_INT, recv_data.data(), recv_counts.data(), recv_displs.data(), MPI_INT, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    // Sort the received data 
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    std::sort(recv_data.begin(), recv_data.end());
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");


    // CORRECTNESS CHECK BEGIN
    CALI_MARK_BEGIN("correctness_check");
    bool globally_sorted = is_globally_sorted(recv_data, rank, num_procs);
    CALI_MARK_END("correctness_check");
    // CORRECTNESS CHECK END

    // Prints result of correctness check, pivots, and oversampling constant
    if (rank == 0) {
        if (globally_sorted) {
            printf("The data is globally sorted.\n");
        } else {
            printf("The data is NOT globally sorted.\n");
        }
        // printf("Pivots: ");
        // for (int i = 0; i < num_procs - 1; i++) {
        //     printf("%d ", pivots[i]);
        // }
        // printf("\n");
        // printf("k: %d\n", k);
    }

    // Prints elements in each rank
    // MPI_Barrier(MPI_COMM_WORLD);
    // for (int i = 0; i < num_procs; i++) {
    //     if (rank == i) {
    //         printf("Rank %d: ", rank);
    //         for (int i = 0; i < total_recv; i++) {
    //             printf("%d ", recv_data[i]);
    //         }
    //         printf("\n");
    //     }
    //     MPI_Barrier(MPI_COMM_WORLD);
    // }

    // Flush Caliper output before finalizing MPI
    mgr.flush();
    mgr.stop();

    MPI_Finalize();
    return 0;
}