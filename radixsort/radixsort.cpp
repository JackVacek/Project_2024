#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

#include <mpi.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

using namespace std;

vector<int> random(size_t size, size_t total_size, int rank) {
    mt19937 gen(time(0) + rank * 1000);
    uniform_int_distribution<> dist(0, total_size);
    vector<int> vec(size);
    generate(vec.begin(), vec.end(), [&]() { return dist(gen); });
    return vec;
}

vector<int> sorted(size_t size, size_t total_size, int rank) {
    vector<int> vec;
    for (size_t i = rank * size; i < size * (rank + 1); i++) {
        vec.push_back(i);
    }
    return vec;
}

vector<int> reversed(size_t size, size_t total_size, int rank) {
    vector<int> vec;
    for (size_t i = total_size - (size * rank); i > total_size - size * (rank + 1); i--) {
        vec.push_back(i);
    }
    return vec;
}

vector<int> perturbed(size_t size, size_t total_size, int rank) {
    vector<int> vec = sorted(size, total_size, rank);
    mt19937 gen(time(0) + rank * 1000);
    uniform_int_distribution<> dist(0, total_size);
    uniform_int_distribution<> perturb(0, 99);
    for (size_t i = 0; i < size; i++) {
        if (perturb(gen) == 0) {
            vec[i] = dist(gen);
        }
    }
    return vec;
}

vector<int> data_init_runtime(int rank, int num_procs, int input_size, string input_type, int local_size) {
    CALI_CXX_MARK_FUNCTION;
    
    vector<int> data;

    if (input_type == "Random") {
        data = random(local_size, input_size, rank);
    } else if (input_type == "Sorted") {
        data = sorted(local_size, input_size, rank);
    } else if (input_type == "ReverseSorted") {
        data = reversed(local_size, input_size, rank);
    } else if (input_type == "1_perc_perturbed") {
        data = perturbed(local_size, input_size, rank);
    }

    return data;
}

void radix_sort(vector<int>& arr) {
    int max = *max_element(arr.begin(), arr.end());

    for (int digit = 1; max / digit > 0; digit *= 10) {
        vector<int> sorted(arr.size());
        int count[10] = {0};

        for (int i = 0; i < arr.size(); ++i)
            count[(arr[i] / digit) % 10]++;
        
        for (int i = 1; i < 10; ++i)
            count[i] += count[i-1];

        for (int i = arr.size() - 1; i >= 0; --i)
            sorted[--count[(arr[i] / digit) % 10]] = arr[i];
        
        arr = sorted;
    }   
}

vector<int> merge_sorted(vector<int> arr, int num_procs, int local_size) {
    vector<vector<int>> parts(num_procs);
    for (int i = 0; i < num_procs; ++i)
        parts[i] = vector<int>(arr.begin() + i * local_size, arr.begin() + (i+1) * local_size);

    vector<int> sorted = parts[0];
    for (int i = 1; i < parts.size(); ++i) {
        vector<int> merged(sorted.size() + parts[i].size());
        merge(sorted.begin(), sorted.end(), parts[i].begin(), parts[i].end(), merged.begin());
        sorted = merged;
    }

    return sorted;
}

bool correctness_check(vector<int> arr) {
    CALI_CXX_MARK_FUNCTION;
    
    for (int i = 1; i < arr.size(); ++i) {
        if (arr[i] < arr[i-1]) {
            return false;
        }
        //cout << arr[i] << " ";
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    CALI_CXX_MARK_FUNCTION;

    MPI_Init(&argc, &argv);
    
    cali::ConfigManager mgr;
    mgr.start();
    
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc < 3) {
        if (rank == 0) {
            cerr << "Usage: " << argv[0]
                      << " <input_size> <input_type>\n"
                      << "input_size: 2^16, 2^18, 2^20, ..., 2^28\n"
                      << "input_type: Sorted, ReverseSorted, Random, 1_perc_perturbed\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    int input_size = atoi(argv[1]);
    string input_type = argv[2];

    int local_size = input_size / num_procs;
    vector<int> local = data_init_runtime(rank, num_procs, input_size, input_type, local_size);

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    radix_sort(local);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    int step = 1;
    while (step < num_procs) {
        if (rank % (2 * step) == 0) {
            int partner = rank + step;
            if (partner < num_procs) {
                int recv_size;
                CALI_MARK_BEGIN("comm");
                CALI_MARK_BEGIN("comm_small");
                MPI_Recv(&recv_size, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                CALI_MARK_END("comm_small");
                CALI_MARK_END("comm");

                vector<int> recv(recv_size);
                CALI_MARK_BEGIN("comm");
                CALI_MARK_BEGIN("comm_large");
                MPI_Recv(recv.data(), recv_size, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                CALI_MARK_END("comm_large");
                CALI_MARK_END("comm");

                CALI_MARK_BEGIN("comp");
                CALI_MARK_BEGIN("comp_large");
                vector<int> merged(local.size() + recv_size);
                merge(local.begin(), local.end(), recv.begin(), recv.end(), merged.begin());
                local = move(merged);
                CALI_MARK_END("comp_large");
                CALI_MARK_END("comp");
            }
        } else {
            int target = rank - step;

            CALI_MARK_BEGIN("comm");
            CALI_MARK_BEGIN("comm_small");
            MPI_Send(&local_size, 1, MPI_INT, target, 0, MPI_COMM_WORLD);
            CALI_MARK_END("comm_small");
            
            CALI_MARK_BEGIN("comm_large");
            MPI_Send(local.data(), local_size, MPI_INT, target, 0, MPI_COMM_WORLD);
            CALI_MARK_END("comm_large");
            CALI_MARK_END("comm");

            break;
        }
        step *= 2;
    }

    if (rank == 0) {
        bool correct = correctness_check(local);
        printf("%s\n", (correct ? "Correct" : "Incorrect"));
    }

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "radix"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
    adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
    adiak::value("scalability", "weak"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 1); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    mgr.stop();
    mgr.flush();

    MPI_Finalize();
}
