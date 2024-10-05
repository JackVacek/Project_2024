# CSCE 435 Group project

## 0. Group number: 1 

Our team will be using Discord to communicate with each other.

## 1. Group members:
1. Jack Vacek - Merge Sort
2. Christopher Vu - Bitonic Sort
3. Victor Pan - Sample Sort
4. Zak Borman - Radix Sort

## 2. Project topic: Parallel Sorting Algorithms

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort: A parallel divide-and-conquer sorting algorithm that creates a bitonic sequence (a series of numbers that monotonically increase and then monotonically decrease) from the input data. To do this, the algorithm uses a network of comparators to perform compare-exchange operations to ensure that the sequence follows this. Then, the algorithm merges the two halves of the sequence to end with a sorted sequence. To implement this sorting algorithm, I will be using the MPI and Grace CPU architecture to parallelize the splitting and compare/swap operations.
- Sample Sort: A parallel sorting algorithm that works by distributing the data into approximately equal-sized buckets and then sorting each bucket. A random sample of elements is then selected from the given data to determine "pivots" or "splitters" that partitions the data into subsets. Each subset is then sorted independently and merged into a fully sorted output. To run this Sample Sort algorithm, the Grace cluster will be used along with the MPI library, which will serve as the framework for communication between processors/nodes and for the parallelization of the sorting process in general. 
- Merge Sort: A sorting algorithm that uses the divide-and-conquer strategy. This is done by splitting the array down into halves until there is one element left, thus sorting it by default. It then merges these sublists back in sorted order until the original list is sorted. This sorting algorithm is parallelized by splitting the original list down to the number of cores and then sorting those lists in parallel, merging the two halves by the "parent" of those processes and going all the way up back to the original list. I will be doing this using MPI to communicate between nodes and Grace cluster to run it.
- Radix Sort: A linear sorting algorithm that sorts elements by processing them digit by digit. Rather than comparing elements directly, Radix Sort distributes the elements into buckets based on each digit’s value. By repeatedly sorting the elements by their significant digits, from the least significant to the most significant, Radix Sort achieves the final sorted order. 

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

**Bitonic Sort**
```
// Initialize MPI
MPI_Init()

// Get number of processes and current process rank
num_processes = MPI_Comm_size(MPI_COMM_WORLD)
rank = MPI_Comm_rank(MPI_COMM_WORLD)

// Get input array and ensure all processes have the size of the input array
If rank == 0:
	Read input array as input_array
	n = size of input_array

// Broadcast n to non-master processes
MPI_Bcast

// Create a new local array based on the number of processors
localSize = num_processors / n

// Evenly distribute chunks of A to each process using MPI_Scatter and localSize
MPI_Scatter

localArray = array of size localSize holding the contents after MPI_Scatter

sort localArray in ascending order if rank / localSize is even otherwise sort descending

num_stages = log_2(P)

for each stage from 1 to num_stages:
	for each step from stage to 0:
		// Getting partner rank to determine what process to compare and exchange with
		partner = rank ^ step
		ascending = true if rank / localSize is even
 
        	if (rank < partner) and ascending:
			// Use MPI_SendRecv to exchange array data with partner
			MPI_SendRecv

			Compare and exchange with partner to ensure ascending order

        	else if (rank > partner) and not ascending:
			// Use MPI_SendRecv to exchange array data with partner
			MPI_SendRecv

			Compare and exchange with partner to ensure descending order

    	// Synchronize all processes after each step
    	MPI_Barrier()

// Gather the fully sorted chunks at the root process
if rank == 0:
	// Gather sorted chunks from all processes into A
	MPI_Gather

	// Print out A
	for element in A:
		print(element)
else:
	// Gather local sorted array
	MPI_Gather

// Finalize MPI
MPI_Finalize
```

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
**Merge Sort**
```
// import mpi, caliper, adiak, etc.
// define my master and worker
// main function
main:
// store the size of the array
sizeOfArray = input from user

// create various variables for number of tasks, workers, source, dest, etc. (and buffers)
numtasks, numworkers, source, dest, timing variables

//make the unsorted array of desired size
for loop to make the overall array

// Initialize MPI
MPI_Init
MPI_Comm_rank
MPI_Comm_size

// how many elements each process is working with
amount = elements / processes

// evenly split the arrays along available processes with MPI_Scatter
MPI_Scatter

// make each process do mergesort locally
localMergeSort

//get all the information from the number of processes into overall list
MPI_Gather

//if the master program, then we will merge all the elements into overall, final array
if master:
	localMergeSort

//make sure everything is synced
MPI_Barrier

// Finalize MPI
MPI_Finalize

// Local merge sort function is the sequential version
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
- Input sizes, input types, and number of processors used in MPI
	- Input Sizes:
		- 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28
	- Input Types:
		- Sorted, Random, Reverse sorted, 1% perturbed
	- Number of Processors:
		- 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024
- Strong scaling (same problem size, increase number of processors/nodes)
	- For strong scaling, we will choose one of the input sizes as the fixed problem size and increase the number of processors according to the range of number of processors provided above. This will be performed for each input size and input type. We will evaluate the effect of strong scaling by comparing runtimes to observe how our algorithms perform with the use of additional processors when maintaining a fixed problem size.
- Weak scaling (increase problem size, increase number of processors)
	- For weak scaling, we will increase the problem size as we increase the number of processors following the given input sizes and processor counts. This will be performed for each input type. By comparing runtimes, we will assess the effect weak scaling has on the performance of each of our algorithms when both the problem size and number of processors grows.
