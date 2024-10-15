  #include <iostream>
  #include <string>
  #include <cmath>
  #include <random>
  #include <algorithm>
  #include <vector>

  #include "mpi.h"

  using namespace std;

  #define MASTER 0

  void printArray(int arr[], int n) {
      for (int i = 0; i < n; ++i) {
          cout << arr[i] << " ";
      }
      cout << endl;
  }

  int main(int argc, char** argv) {
      // Initializing MPI
      MPI_Init(&argc, &argv);

      // Get number of processes and current process rank
      int num_processes, rank, n;
      MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      n = stoi(argv[1]);

      vector<int> input_array(n);
      if (rank == 0) {
          srand(0);
          for (int i = 0; i < n; i++) {
              input_array[i] = rand() % (n/4);
          }
      }
      
      // Create a new local array based on the number of processors
      int localSize = n / num_processes;
      vector<int> localArray(localSize);

      // Distribute the input array to all processes
      MPI_Scatter(input_array.data(), localSize, MPI_INT, localArray.data(), localSize, MPI_INT, MASTER, MPI_COMM_WORLD);
      sort(localArray.begin(), localArray.end());

      int num_stages = log2(num_processes);

      for (int stage = 1; stage <= num_stages; stage++) {
          for (int step = stage; step > 0; step--) {
              int partner_rank = rank ^ (1 << (step - 1));

              // Calculate the group number and the direction of the sort
              int group_size = 1 << stage;
              int group = rank / group_size;
              bool ascending = (group % 2 == 0);

              vector<int> partner_arr(localSize);
              MPI_Sendrecv(localArray.data(), localSize, MPI_INT, partner_rank, 0, partner_arr.data(), localSize, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
              if(rank < partner_rank && ascending || rank > partner_rank && !ascending) {
                  for (int i = 0; i < localSize; i++) {
                      localArray[i] = merged[i];
                  }
              } else {
                  for (int i = 0; i < localSize; i++) {
                      localArray[i] = merged[i + localSize];
                  }
              }
          }
          MPI_Barrier(MPI_COMM_WORLD);
      }

      MPI_Gather(localArray.data(), localSize, MPI_INT, input_array.data(), localSize, MPI_INT, MASTER, MPI_COMM_WORLD);

      if (rank == MASTER) {
          bool sorted = true;
          for (int i = 1; i < n; i++) {
              if (input_array[i] < input_array[i - 1]) {
                  sorted = false;
                  break;
              }
          }

          if (sorted) {
              cout << "Array is sorted" << endl;
          } else {
              cout << "Array is not sorted" << endl;
          }
      }

      MPI_Finalize();
      return 0;
  }