#include <iostream>
#include <vector>
#include <algorithm>

#include <mpi.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

using namespace std;

// Local counting sort for one digit
void counting_sort(vector<int>& arr, int digit) {
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
            std::cerr << "Usage: " << argv[0]
                      << " <input_size> <input_type>\n"
                      << "input_size: 2^16, 2^18, 2^20, ..., 2^28\n"
                      << "input_type: Sorted, ReverseSorted, Random, 1_perc_perturbed\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    int input_size = atoi(argv[1]);
    string input_type = argv[2];
    
    CALI_MARK_BEGIN("data_init_runtime");
    vector<int> arr(input_size);
    if (rank == 0) {
        srand(time(NULL));
        for (int i = 0; i < input_size; ++i) {
            if (input_type == "Sorted" || input_type == "1_perc_perturbed")
                arr[i] = i;
            else if (input_type == "ReverseSorted")
                arr[i] = input_size - i - 1;
            else if (input_type == "Random")
                arr[i] = rand() % input_size;
        }
        if (input_type == "1_perc_perturbed") {
            for (int i = 0; i < input_size / 100; ++i)
                swap(arr[rand() % input_size], arr[rand() % input_size]);
        }
    }
    CALI_MARK_END("data_init_runtime");

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Bcast(&input_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    int local_size = input_size / num_procs;
    vector<int> local(local_size);

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Scatter(arr.data(), local_size, MPI_INT, local.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    // Radix sort
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    int max = *max_element(local.begin(), local.end());
    for (int digit = 1; max / digit > 0; digit *= 10)
        counting_sort(local, digit);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    vector<int> global(input_size);

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Gather(local.data(), local_size, MPI_INT, global.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if (rank == 0) {
        // Merge sorted local arrays
        vector<vector<int>> parts(num_procs);
        for (int i = 0; i < num_procs; ++i)
            parts[i] = vector<int>(global.begin() + i * local_size, global.begin() + (i+1) * local_size);

        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");
        vector<int> sorted = parts[0];
        for (int i = 1; i < parts.size(); ++i) {
            vector<int> merged(sorted.size() + parts[i].size());
            merge(sorted.begin(), sorted.end(), parts[i].begin(), parts[i].end(), merged.begin());
            sorted = merged;
        }
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");
        
        CALI_MARK_BEGIN("correctness_check");
        for (int i = 1; i < sorted.size(); ++i) {
            if (sorted[i] < sorted[i-1]) {
                cout << "Incorrect" << endl;
                break;
            }
        }
        cout << "Correct" << endl;
        CALI_MARK_END("correctness_check");
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
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 1); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    mgr.stop();
    mgr.flush();

    MPI_Finalize();
}
