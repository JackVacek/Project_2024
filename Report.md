# CSCE 435 Group project

## 0. Group number: 1 

### **Communicate through Discord**

## 1. Group members:
1. Jack Vacek - Merge Sort
2. Christopher Vu - Bitonic Sort
3. Victor Pan - Sample Sort
4. Zak Borman - Radix Sort

## 2. Project topic: Parallel Sorting Algorithms

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort: A parallel sorting algorithm that works by selecting a random sample of elements from the given data to determine "pivots" or "splitters" that partitions the data into subsets. Each subset is then sorted independently and merged into a fully sorted output.
- Merge Sort:
- Radix Sort: A linear sorting algorithm that sorts elements by processing them digit by digit. Rather than comparing elements directly, Radix Sort distributes the elements into buckets based on each digit’s value. By repeatedly sorting the elements by their significant digits, from the least significant to the most significant, Radix Sort achieves the final sorted order. 

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

**Sample Sort**
```
// Initialize MPI
MPI_Init

// Get number of processes and current process rank
num_processes = MPI_Comm_size(MPI_COMM_WORLD)
rank = MPI_Comm_rank(MPI_COMM_WORLD)

// Get input array and ensure all processes have the size of the input array
If rank == 0:
	Read input array as input_array
	n = size of input_array

// Broadcast n to non-master processes
MPI_Bcast

// Split input array into buckets and send them to each process depending on calculated bucket sizes and displacements
Calculate bucket sizes of each process and displacements

// Scatter input_array to each process
MPI_Scatterv

// Sort the buckets in each process locally
Sort local_bucket

// Calculate s and select s local samples from each process
s = number of samples
Select s evenly spaced elements from each processes' local_bucket as samples

// Gather all local samples at the master process
MPI_Gather

// Sort samples then select pivots
If rank == 0:
	Sort gathered samples
	Select num_processes - 1 pivots from gathered samples

// Send pivots to non-master processes
MPI_Bcast

// Reorganize buckets based on pivots
Initialize num_processes empty buckets
For each element in local_bucket:
    Determine the appropriate bucket based on pivots
    Place element into its corresponding bucket

// Get number of elements to send to each process in order for each process to know the number of elements they are receiving from each process
Prepare new bucket sizes
MPI_Alltoall

// Prepare and send new buckets based on pivots to its corresponding process
Calculate send and receive displacements
Flatten new buckets based on pivots in a single array
MPI_Alltoallv

// Sort received data from pivots of each processes' bucket
Sort received_data

// Gather sorted buckets in the master process combine them into a fully sorted array
MPI_Gatherv

// Finalize MPI
MPI_Finalize
```

**Radix Sort**
```
Initialize MPI
Get total number of processes (P) and current process rank (rank)

if rank == 0:
    Read input array A
    Broadcast the size of A to all processes
else:
    Receive the size of A

Distribute chunks of A to each process (scatter)
Each process now has a local chunk A_local

for each digit position (starting from least significant to most significant):
    
    // Step 1: Local counting sort by the current digit
    Initialize local_count array to count occurrences of each digit (0-9)
    for each element in A_local:
        Extract the current digit
        Increment local_count[current_digit]

    // Step 2: Gather the counts from all processes to the root
    if rank == 0:
        global_count = Gather local_count from all processes
    else:
        Send local_count to root process

    if rank == 0:
        // Step 3: Compute global offsets based on global_count
        Calculate the start index for each process based on global_count
        Send offsets to all processes

    // Step 4: Each process rearranges its local data based on the offsets
    Receive offsets from root
    Initialize local_sorted array
    for each element in A_local:
        Extract the current digit
        Place element in the correct position in local_sorted using offsets

    // Synchronize all processes
    Barrier()

// Step 5: Gather sorted chunks at the root process
if rank == 0:
    Gather sorted chunks from all processes into A
else:
    Send local_sorted to root process

if rank == 0:
    Print the fully sorted array A

Finalize MPI
```

### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
