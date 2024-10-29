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

**Algorithms Descriptions:**
- Bitonic Sort: A parallel divide-and-conquer sorting algorithm that creates a bitonic sequence (a series of numbers that monotonically increase and then monotonically decrease) from the input data. To do this, the algorithm uses a network of comparators to perform compare-exchange operations to ensure that the sequence follows this. Then, the algorithm merges the two halves of the sequence to end with a sorted sequence. To implement this sorting algorithm, I will be using the MPI and Grace CPU architecture to parallelize the splitting and compare/swap operations.
- Sample Sort: A parallel sorting algorithm that works by distributing the data into approximately equal-sized buckets and then sorting each bucket. A random sample of elements is then selected from the given data to determine "pivots" or "splitters" that partitions the data into subsets. Each subset is then sorted independently and merged into a fully sorted output. To run this Sample Sort algorithm, the Grace cluster will be used along with the MPI library, which will serve as the framework for communication between processors/nodes and for the parallelization of the sorting process in general. 
- Merge Sort: A sorting algorithm that uses the divide-and-conquer strategy. This is done by splitting the array down into halves until there is one element left, thus sorting it by default. It then merges these sublists back in sorted order until the original list is sorted. This sorting algorithm is parallelized by splitting the original list down to the number of cores and then sorting those lists in parallel, merging two processes and going all the way up back to the original list. I will be doing this using MPI to communicate between processes and HPRC Grace CPU architecture to parallelize the sorting of this algorithm.
- Radix Sort: A linear sorting algorithm that sorts elements by processing them digit by digit. Rather than comparing elements directly, Radix Sort distributes the elements into buckets based on each digit’s value. By repeatedly sorting the elements by their significant digits, from the least significant to the most significant, Radix Sort achieves the final sorted order. To implement this algorithm, I will be using the MPI library to facilitate communication between processes and the Grace CPU architecture to parallelize this.

**Source Code Descriptions:**
- Bitonic Sort: In this implentation of Bitionic Sort, MPI is initialized and the number of processes and rank of the current process is obtained. It also accepts two command line arguments: n, the total number of elements we're going to generate & sort, and input_type, which (Random, Sorted, ReverseSorted, or 1% perturbed). Based on this, each process independently creates a local array of size n/num_processes and initializes the array based on the input type. With this, each process has its own local data set, which forms the entire input if put together. Next, each process sorts its own localArray in ascending order to set up the merging procedure in bitonic sort. After that, we go into our nested loop, which goes through a series of log2(num_processes) stages and steps. In each step, the current process and it's partner are paired up to compare and exchange with each other using the XOR operator, which lets each process pair with a different partner in each step. After that, each process sends over its local array with its partner and vice versa. The two sorted arrays are merged together in a way that the sorted property remains and, depending on the direction of the sort, the current process's rank and its partner's rank, either the lower half or the upper half of the merged array is kept. After each stage, there is a barrier that waits for all processes to finish before proceeding to the next one. Finally, in the correctness check, each process checks to see if its local array is sorted and that the next rank is larger than the current one. 
- Sample Sort: In this implementation of Sample Sort, each process independently generates its own portion of the input data based on the specified type (random, sorted, reverse sorted, or 1% perturbed). Each processor has its own local data set where collectively, the entire input is formed. Since the data is already split between processors, the first step of Sample Sort is complete after generation. Next, each processor sorts its local data. After sorting, samples are chosen for each processor's local data. Oversampling is utilized in this algorithm for better scaling and to provide samples that better represent the data itself. Each processor selects *k * num_procs* evenly spaced samples from its sorted local data, where *k* is the oversampling factor and *num_procs* is the number of processors. The local samples are then gathered from all processors into a global sample array. The global sample array is then sorted, and *num_procs - 1* pivots are selected at evenly spaced intervals to divide the data range into num_procs buckets. After selecting the pivots from the samples, each processor partitions its sorted local data into these buckets based on the pivots. The buckets are then exchanged between the processors to ensure each processor gets the values with their respective bucket. Each processor now has the data that falls within its assigned range based off of the selected pivots. After the data exchange, each processor performs a final sort on its local dataset and the data as a whole is checked to ensure it is sorted across all processors.
- Merge Sort: In this implementation of Merge Sort, the user passes in through the command line arguments the number of processes, size of the array, and what type of input data (random, sorted, reverse sorted, or 1% perturbed). After reading these inputs, the choose_array_type method is called in each process to generate the data to be sorted for their subsection of the overall array. This is due to having more data than processes, so each process receives a smaller version of the overall problem to parallelize the computation. Once this data is generated and populated into the array, they will all call a sequential version of merge sort. Each process will individually sort their local array and then merge together in a tree and pair-like fashion. We first have a step variable to keep track of the distance between each task that we are merging and then a loop to make sure that we process each tree level. It is set up so that tasks that are divisible by *(2 * step)* are the receiving processes and otherwise are the sending processes, which send their data to these receivers to merge. Next there is a check to make sure that the sender is valid and if it is, it first gets the size of the array that it is receiving from the sender, makes a new list of the correct size to be populated by that sender, and then populates it. After this, it creates a new array to be populated with both lists and then calls the merge function to sort them and then updates the size and data of the local array. If the process was a sender, then it finds where it needs to send the data to and then sends its size and data to that target process, breaking after because it no longer needs to send more, as some of the previous receivers will be turned into senders in the next loop. Lastly, it doubles the step, as the distance between processes has now doubled. Finally, The master process does a correctness check on the entire array, as they have all been sorted into this process. 
- Radix Sort: In this implementation of Radix Sort, MPI is used to parallelize the sorting process by distributing the input data across multiple processes. The program accepts two command-line arguments: input_size (total number of elements) and input_type (Random, Sorted, ReverseSorted, or 1% perturbed). Each process generates a local array of size input_size/num_procs based on the input type and performs a sequential radix sort on its subset, sorting elements digit by digit using counting sort for each significant digit. After the local sort, processes participate in a step-wise merging phase where pairs of processes exchange and merge their data. In each step, processes with ranks divisible by 2 * step receive data from their partners, merge it with their local array, and update their data, while others send their data and exit further merging. This continues until all data is consolidated in the process with rank 0, which performs a correctness check to ensure the global array is sorted. Performance markers with Caliper and metadata logging via Adiak capture details about the execution, such as the algorithm type, input size, and scalability metrics. Finally, MPI is finalized, and the program terminates.

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

**Bitonic Sort**
```
// Initialize MPI
MPI_Init()

// Get number of processes and current process rank
num_procs = MPI_Comm_size(MPI_COMM_WORLD)
rank = MPI_Comm_rank(MPI_COMM_WORLD)

// Determine total size of input and input type
totalSize = Command-line input
input_type = Command-line input

// Calculate localSize and initialize localArray based on input_type
localSize = totalSize / num_procs
if input_type == "Random":
    localArray = random(localSize, totalSize, rank)
else if input_type == "Sorted":
    localArray = sorted(localSize, totalSize, rank)
else if input_type == "ReverseSorted":
    localArray = reversed(localSize, totalSize, rank)
else if input_type == "1_perc_perturbed":
    localArray = perturbed(localSize, totalSize, rank)

// Sort localArray in ascending order
sort(localArray)

// Calculate number of stages for bitonic sort
num_stages = log2(num_procs)

// Perform bitonic sort stages
for stage from 1 to num_stages:
    for step from stage down to 1:
        partner_rank = rank XOR (1 << (step - 1))
        
        // Determine group size, group number, and sorting direction (ascending/descending)
        group_size = 1 << stage
        group = rank / group_size
        ascending = (group % 2 == 0)

        // Exchange data with partner
        partner_arr = array of size localSize
        MPI_Sendrecv(localArray, partner_rank, partner_arr, partner_rank)

        // Merge localArray and partner_arr
        merged = merge localArray and partner_arr maintaining sorted order

        // Copy merged array back to localArray based on direction of the sort
        if rank < partner_rank and ascending or rank > partner_rank and not ascending:
            localArray = first half of merged
        else:
            localArray = second half of merged

    // Synchronize all processes after each step
    MPI_Barrier(MPI_COMM_WORLD)

// Check if data is globally sorted
if rank == 0:
    if is_globally_sorted(localArray):
        print "The data is globally sorted."
    else:
        print "The data is NOT globally sorted."

// Gather metadata and finalize MPI
MPI_Finalize()
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
CALI_CXX_MARK_FUNCTION
Set up variables for type of array and size of array
Read in the type and size

Set up variables for number of tasks, tasks id, and rc

Set up caliper regions

MPI_INIT()
MPI_Comm_rank()
MPI_Comm_size()

calculate localarray size
set up localarray

start caliper

//error handle
if numtasks < 2
	MPI_Abort()

calculate offset for generating numbers in local array
choose_array_type() - choose the data we want to include in the local array

mergeSort() - call mergesort on each local array

set step variable
while step < numtasks:
	if taskid % (2 * step) == 0: //receiving process
		if taskid + step < numtasks:
			get size of sending array
			MPI_Recv()
			get the data from the sending array
			MPI_Recv()
			create a new array of the size of both current receiving and the sending array
			copy their data into the new array
			merge()
			update localarray to be newarray
	else: //sending process
		calculate the target process (process we are sending the info to)
		Send the size of the array
		MPI_Send()
		send the data of the array
		MPI_Send()
		break
	step *= 2

if task == Master:
	isCorrect = False by default
	correctness_checker()
	print out if the algorithm produced the correct output

stop the caliper
put all of the metadata from adiak
delete localArrays
MPI_Finalize()
```
**Radix Sort**
```
// Initialize MPI
MPI_Init()

// Get the number of processes and the rank of the current process
num_procs = MPI_Comm_size(MPI_COMM_WORLD)
rank = MPI_Comm_rank(MPI_COMM_WORLD)

// Parse input arguments for input size and type
if rank == 0:
    if missing_arguments():
        print_usage_and_finalize()
        return

input_size = atoi(argv[1])
input_type = argv[2]

// Calculate the size of the local array for each process
local_size = input_size / num_procs

// Initialize local data based on input type
local_array = data_init_runtime(rank, num_procs, input_size, input_type, local_size)

// Perform radix sort on the local array
radix_sort(local_array)

// Merge sorted arrays from all processes using MPI
step = 1
while step < num_procs:
    if rank % (2 * step) == 0:
        partner = rank + step
        if partner < num_procs:
            // Receive size of partner’s array
            MPI_Recv(recv_size, 1, MPI_INT, partner, 0)

            // Receive partner’s array data
            recv_array = allocate(recv_size)
            MPI_Recv(recv_array, recv_size, MPI_INT, partner, 0)

            // Merge local array with received array
            merged_array = merge(local_array, recv_array)
            local_array = merged_array
    else:
        // Determine the target process to send data to
        target = rank - step

        // Send the size of the local array
        MPI_Send(local_size, 1, MPI_INT, target, 0)

        // Send the local array data
        MPI_Send(local_array, local_size, MPI_INT, target, 0)
        
        break  // Stop further participation in merging
    step *= 2

// If rank 0, check correctness of the final merged array
if rank == 0:
    correct = correctness_check(local_array)
    print("Correct" if correct else "Incorrect")

// Finalize MPI
MPI_Finalize()
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

### 3a. Caliper instrumentation
Please use the caliper build `/scratch/group/csce435-f24/Caliper/caliper/share/cmake/caliper` 
(same as lab2 build.sh) to collect caliper files for each experiment you run.

Your Caliper annotations should result in the following calltree
(use `Thicket.tree()` to see the calltree):
```
main
|_ data_init_X      # X = runtime OR io
|_ comm
|    |_ comm_small
|    |_ comm_large
|_ comp
|    |_ comp_small
|    |_ comp_large
|_ correctness_check
```

Required region annotations:
- `main` - top-level main function.
    - `data_init_X` - the function where input data is generated or read in from file. Use *data_init_runtime* if you are generating the data during the program, and *data_init_io* if you are reading the data from a file.
    - `correctness_check` - function for checking the correctness of the algorithm output (e.g., checking if the resulting data is sorted).
    - `comm` - All communication-related functions in your algorithm should be nested under the `comm` region.
      - Inside the `comm` region, you should create regions to indicate how much data you are communicating (i.e., `comm_small` if you are sending or broadcasting a few values, `comm_large` if you are sending all of your local values).
      - Notice that auxillary functions like MPI_init are not under here.
    - `comp` - All computation functions within your algorithm should be nested under the `comp` region.
      - Inside the `comp` region, you should create regions to indicate how much data you are computing on (i.e., `comp_small` if you are sorting a few values like the splitters, `comp_large` if you are sorting values in the array).
      - Notice that auxillary functions like data_init are not under here.
    - `MPI_X` - You will also see MPI regions in the calltree if using the appropriate MPI profiling configuration (see **Builds/**). Examples shown below.

All functions will be called from `main` and most will be grouped under either `comm` or `comp` regions, representing communication and computation, respectively. You should be timing as many significant functions in your code as possible. **Do not** time print statements or other insignificant operations that may skew the performance measurements.

### **Nesting Code Regions Example** - all computation code regions should be nested in the "comp" parent code region as following:
```
CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_small");
sort_pivots(pivot_arr);
CALI_MARK_END("comp_small");
CALI_MARK_END("comp");

# Other non-computation code
...

CALI_MARK_BEGIN("comp");
CALI_MARK_BEGIN("comp_large");
sort_values(arr);
CALI_MARK_END("comp_large");
CALI_MARK_END("comp");
```

### **Calltree Example**:
```
# MPI Mergesort
4.695 main
├─ 0.001 MPI_Comm_dup
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Finalized
├─ 0.000 MPI_Init
├─ 0.000 MPI_Initialized
├─ 2.599 comm
│  ├─ 2.572 MPI_Barrier
│  └─ 0.027 comm_large
│     ├─ 0.011 MPI_Gather
│     └─ 0.016 MPI_Scatter
├─ 0.910 comp
│  └─ 0.909 comp_large
├─ 0.201 data_init_runtime
└─ 0.440 correctness_check
```

**Bitonic Sort Example Calltree**
```
1.68131 main
├─ 0.00004 MPI_Init
├─ 0.00286 data_init_runtime
├─ 0.06696 comp
│  ├─ 0.06663 comp_large
│  └─ 0.00008 comp_small
├─ 0.02028 comm
│  ├─ 0.00941 comm_large
│  │  └─ 0.00926 MPI_Sendrecv
│  └─ 0.01073 MPI_Barrier
├─ 0.00065 correctness_check
│  ├─ 0.00013 MPI_Send
│  ├─ 0.00003 MPI_Recv
│  └─ 0.00009 MPI_Allreduce
├─ 0.00000 MPI_Finalize
├─ 0.00001 MPI_Initialized
├─ 0.00001 MPI_Finalized
└─ 0.00083 MPI_Comm_dup
```

**Sample Sort Example Calltree**
```
1.686 main
├─ 0.000 MPI_Init
├─ 0.010 data_init_runtime
├─ 0.108 comp
│  ├─ 0.107 comp_large
│  └─ 0.001 comp_small
├─ 0.016 comm
│  ├─ 0.012 comm_small
│  │  ├─ 0.011 MPI_Allgather
│  │  └─ 0.001 MPI_Alltoall
│  └─ 0.004 comm_large
│     └─ 0.004 MPI_Alltoallv
├─ 0.021 correctness_check
│  ├─ 0.000 MPI_Send
│  ├─ 0.020 MPI_Recv
│  └─ 0.000 MPI_Allreduce
├─ 0.000 MPI_Finalize
├─ 0.000 MPI_Initialized
├─ 0.000 MPI_Finalized
└─ 0.000 MPI_Comm_dup
```

**Merge Sort Example Calltree**
```
3.38795 main
├─ 0.23467 MPI_Comm_dup
├─ 0.00001 MPI_Finalize
├─ 0.00001 MPI_Finalized
├─ 0.00006 MPI_Init
├─ 0.00001 MPI_Initialized
├─ 0.00594 comm
│  ├─ 0.00046 comm_large
│  │  ├─ 0.00023 MPI_Recv
│  │  └─ 0.00033 MPI_Send
│  └─ 0.00544 comm_small
│     ├─ 0.01012 MPI_Recv
│     └─ 0.00036 MPI_Send
├─ 0.00005 comp
│  └─ 0.00004 comp_large
├─ 0.00046 correctness_check
└─ 0.00005 data_init_runtime
```

**Radix Sort Example Calltree**
```
0.48409 main
├─ 0.02730 MPI_Comm_dup
├─ 0.00001 MPI_Finalize
├─ 0.00001 MPI_Finalized
├─ 0.00004 MPI_Init
├─ 0.00001 MPI_Initialized
├─ 0.00127 comm
│  ├─ 0.00115 comm_large
│  │  ├─ 0.00083 MPI_Recv
│  │  └─ 0.00143 MPI_Send
│  └─ 0.00010 comm_small
│     ├─ 0.00013 MPI_Recv
│     └─ 0.00005 MPI_Send
├─ 0.12135 comp
│  └─ 0.12131 comp_large
├─ 0.00769 correctness_check
└─ 0.03557 data_init_runtime
```

### 3b. Collect Metadata

Have the following code in your programs to collect metadata:
```
adiak::init(NULL);
adiak::launchdate();    // launch date of the job
adiak::libraries();     // Libraries used
adiak::cmdline();       // Command line used to launch the job
adiak::clustername();   // Name of the cluster
adiak::value("algorithm", algorithm); // The name of the algorithm you are using (e.g., "merge", "bitonic")
adiak::value("programming_model", programming_model); // e.g. "mpi"
adiak::value("data_type", data_type); // The datatype of input elements (e.g., double, int, float)
adiak::value("size_of_data_type", size_of_data_type); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
adiak::value("input_size", input_size); // The number of elements in input dataset (1000)
adiak::value("input_type", input_type); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
adiak::value("num_procs", num_procs); // The number of processors (MPI ranks)
adiak::value("scalability", scalability); // The scalability of your algorithm. choices: ("strong", "weak")
adiak::value("group_num", group_number); // The number of your group (integer, e.g., 1, 10)
adiak::value("implementation_source", implementation_source); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").
```

They will show up in the `Thicket.metadata` if the caliper file is read into Thicket.

**Bitonic Sort Example Metadata**
```
launchdate: 1729089590
libraries: /scratch/group/csce435-f24/Caliper/caliper/lib64/libcaliper.so.2 + more...
cmdline: ['./bitonicsort', '4194304', 'Random']
cluster: c
algorithm: bitonic
programming_model: mpi
data_type: int
size_of_data_type: 4
input_size: 4194304
input_type: Random
num_procs: 32
scalability: strong
group_num: 1
implementation_source: handwritten
```

**Sample Sort Example Metadata**
```
launchdate: 1729125502
libraries: /scratch/group/csce435-f24/Caliper/caliper/lib64/libcaliper.so.2 + more...
cmdline: ['./samplesort', '4194304', 'Random']
cluster: c
algorithm: sample
programming_model: mpi
data_type: int
size_of_data_type: 4
input_size: 4194304
input_type: Random
num_procs: 32
scalability: strong
group_num: 1
implementation_source: handwritten
```

**Merge Sort Example Metadata**
```
launchdate: 1729829519
libraries: /scratch/group/csce435-f24/Caliper/caliper/lib64/libcaliper.so.2 + more...
cmdline: ['./mergesort', '65536', 'Random']
cluster: c
algorithm: merge
programming_model: mpi
data_type: int
size_of_data_type: 4
input_size: 65536
input_type: Random
num_procs: 1024
scalability: weak
group_num: 1
implementation_source: online
```

**Radix Sort Example Metadata**
```
launchdate: 1730158958
libraries: /scratch/group/csce435-f24/Caliper/caliper/lib64/libcaliper.so.2 + more...
cmdline: ['./radixsort', '1048576', 'Random']
cluster: c
algorithm: radix
programming_model: mpi
data_type: int
size_of_data_type: 4
input_size: 1048576
input_type: Random
num_procs: 2
scalability: weak
group_num: 1
implementation_source: handwritten
```

### **See the `Builds/` directory to find the correct Caliper configurations to get the performance metrics.** They will show up in the `Thicket.dataframe` when the Caliper file is read into Thicket.
## 4. Performance evaluation

Include detailed analysis of computation performance, communication performance. 
Include figures and explanation of your analysis.

### 4a. Vary the following parameters
For input_size's:
- 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28

For input_type's:
- Sorted, Random, Reverse sorted, 1%perturbed

MPI: num_procs:
- 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024

This should result in 4x7x10=280 Caliper files for your MPI experiments.

### 4b. Hints for performance analysis

To automate running a set of experiments, parameterize your program.

- input_type: "Sorted" could generate a sorted input to pass into your algorithms
- algorithm: You can have a switch statement that calls the different algorithms and sets the Adiak variables accordingly
- num_procs: How many MPI ranks you are using

When your program works with these parameters, you can write a shell script 
that will run a for loop over the parameters above (e.g., on 64 processors, 
perform runs that invoke algorithm2 for Sorted, ReverseSorted, and Random data).  

### 4c. You should measure the following performance metrics
- `Time`
    - Min time/rank
    - Max time/rank
    - Avg time/rank
    - Total time
    - Variance time/rank

**Bitonic Sort Main Graphs**
![Main Times 65536](./bitonicsort/main_graphs/main_input_size_65536.png)
![Main Times 262144](./bitonicsort/main_graphs/main_input_size_262144.png)
![Main Times 1048576](./bitonicsort/main_graphs/main_input_size_1048576.png)
![Main Times 4194304](./bitonicsort/main_graphs/main_input_size_4194304.png)
![Main Times 16777216](./bitonicsort/main_graphs/main_input_size_16777216.png)
![Main Times 67108864](./bitonicsort/main_graphs/main_input_size_67108864.png)
![Main Times 268435456](./bitonicsort/main_graphs/main_input_size_268435456.png)
![Main Variance Random 65536](./bitonicsort/part4_graphs/Random/main_Variance_Random_65536.png)
![Main Variance Random 268435456](./bitonicsort/part4_graphs/Random/main_Variance_Random_268435456.png)

**Bitonic Sort Comm Graphs**
![Comm Random Input Size 65536](./bitonicsort/part4_graphs/Random/comm_Random_inputsize_65536.png)
![Comm Random Input Size 268435456](./bitonicsort/part4_graphs/Random/comm_Random_inputsize_268435456.png)
![Comm Variance Random 65536](./bitonicsort/part4_graphs/Random/comm_Variance_Random_65536.png)
![Comm Variance Random 268435456](./bitonicsort/part4_graphs/Random/comm_Variance_Random_268435456.png)

**Bitonic Sort Comp Large Graphs**
![Comp Large Random Input Size 65536](./bitonicsort/part4_graphs/Random/comp_large_Random_inputsize_65536.png)
![Comp Large Random Input Size 268435456](./bitonicsort/part4_graphs/Random/comp_large_Random_inputsize_268435456.png)
![Comp Large Variance Random 65536](./bitonicsort/part4_graphs/Random/comp_large_Variance_Random_65536.png)
![Comp Large Variance Random 268435456](./bitonicsort/part4_graphs/Random/comp_large_Variance_Random_268435456.png)

**Analysis**
- In these graphs for bitonic sort that showcase the main function's runtime as a function of # of processes vs. time for each input type, we can see that for smaller inputs, there's an overall increasing trend up until around 2^24 array size. This can be explained due to the fact that there's overhead with MPI initializations, data generation, correctness check, etc. For these input sizes, it outweighs the computation speedup of the algorithm when we add more processes since the total time is so small already. As for the different input types, it seems to go in the order of increasing to decreasing for: perturbed, random, sorted, reversed. This pattern decreases and it's less visible as we add more processes. This can be due to the fact that our random generator library has more overhead (and perturbed essentially sorts it and then perturbs it causing it to be larger). Additionally, when we start using multiple nodes, there can be overhead associated with MPI communication between nodes as well (which is typically larger). However, as we add more processes, and the time seems to decrease in a seemingly exponential decay pattern. This makes sense since the main array can be split up into more processes and the sorting takes less time by that factor. This pattern seems to be approximately the same across max, min, and avg time per rank. For Total time (for the main function), the trends mostly linearly increase since there are more processes, and even though each process does less total work, there is still overhead that each individual process must do. Regarding communication, it seems like there's a lot of random noise. However, this is likely because there a lot of people using the HPRC Grace cluster, which could be causing network latency and communication latency. For computation time, it seems like for both the smallest and largest array sizes, the time decreases at an exponential rate as we add more processes. For Variance time/rank in main max size, you can see that it hovers around 0 until about 1024 where the variance spikes to 0.005. There are larger spikes in the smaller 2^16 input size, which can be logically explained simply because noise is amplified when the computation time is so small for this array size. However, this can be negligible since this is a value around 0. When we look at comm, it seems like the variance for the larger input size has a higher value, which can make sense since I'm sending and receiving more data per rank. The same explanation for variations can be made for comp_large.

**Sample Sort Main Graphs**
![Main Times 65536](./samplesort/main_graphs/main_input_size_65536.png)
![Main Times 262144](./samplesort/main_graphs/main_input_size_262144.png)
![Main Times 1048576](./samplesort/main_graphs/main_input_size_1048576.png)
![Main Times 4194304](./samplesort/main_graphs/main_input_size_4194304.png)
![Main Times 16777216](./samplesort/main_graphs/main_input_size_16777216.png)
![Main Times 67108864](./samplesort/main_graphs/main_input_size_67108864.png)
![Main Times 268435456](./samplesort/main_graphs/main_input_size_268435456.png)
![Main Variance Random 65536](./samplesort/part4_graphs/Random/main_Variance_Random_65536.png)
![Main Variance Random 268435456](./samplesort/part4_graphs/Random/main_Variance_Random_268435456.png)

**Sample Sort Comm Graphs**
![Comm Random Input Size 65536](./samplesort/part4_graphs/Random/comm_Random_inputsize_65536.png)
![Comm Random Input Size 268435456](./samplesort/part4_graphs/Random/comm_Random_inputsize_268435456.png)
![Comm Variance Random 65536](./samplesort/part4_graphs/Random/comm_Variance_Random_65536.png)
![Comm Variance Random 268435456](./samplesort/part4_graphs/Random/comm_Variance_Random_268435456.png)

**Sample Sort Comp Large Graphs**
![Comp Large Random Input Size 65536](./samplesort/part4_graphs/Random/comp_large_Random_inputsize_65536.png)
![Comp Large Random Input Size 268435456](./samplesort/part4_graphs/Random/comp_large_Random_inputsize_268435456.png)
![Comp Large Variance Random 65536](./samplesort/part4_graphs/Random/comp_large_Variance_Random_65536.png)
![Comp Large Variance Random 268435456](./samplesort/part4_graphs/Random/comp_large_Variance_Random_268435456.png)

**Analysis:**
- For these graphs, we can see that the trends change as the input size increases. This is because as the input size increases, the communication overhead begins to go away. For the first three graphs, we see that the times it take for communication overtakes the computation time. Since there are more processes to communicate between, the main time would increase as the number of processes increases. In the fourth graph, you can see that the communcation times and computation times begin to even out where computation times begin to matter more. This is noticeable throughout Min, Max, and Avg times per rank. The trend becomes something more linear and constant. For the final three graphs, we can see that computation completely overtakes communication times and the trend changes. We see that the time it takes to compute the data decreases as the number of processes increases. This is because the data is split up more and more, so the computation time decreases. The average time per rank gives a better look at these trends, but minimum and maximum times follows around the same trend with some outliers. For total time, the trends are mostly linearly increasing because there are more processes. More processes means for times to add for each process, so the total time increases. For variance time per rank, the trends are overall consistent except for some spikes for some jobs. This is normal since the sorting times should generally be consistent, but there are some outliers that can cause the variance to increase. In the comp_large graphs, we find that the variance is large in the beginning, but becomes more consistent as the number of processors increases. This is because when there are two processors, Sample Sort spends a lot more time sorting its buckets due to the nature of the algorithm. However, as the number of processors increases, the time spent sorting the buckets decreases, leading to less variance in the computation time. In the comm graphs, we see that for the largest input size, it has the most variance. This is because there isn't much to communicate so there is likely other noise affecting this. For the smallest input size, the variance is much lower except for at the end because the there is a drastic increase in the amount of data being communicated.

**Merge Sort Graphs**
![Main Times 65536](./mergesort/Graphs/part5_graphs/strong_scaling/main_65536.png)
![Main Times 262144](./mergesort/Graphs/part5_graphs/strong_scaling/main_262144.png)
![Main Times 1048576](./mergesort/Graphs/part5_graphs/strong_scaling/main_1048576.png)
![Main Times 4194304](./mergesort/Graphs/part5_graphs/strong_scaling/main_4194304.png)
![Main Times 16777216](./mergesort/Graphs/part5_graphs/strong_scaling/main_16777216.png)
![Main Times 67108864](./mergesort/Graphs/part5_graphs/strong_scaling/main_67108864.png)
![Main Times 268435456](./mergesort/Graphs/part5_graphs/strong_scaling/main_268435456.png)
![Main Variance Random 65536](./mergesort/Graphs/part4_graphs/Random/main_Variance_Random_65536.png)
![Main Variance Random 268435456](./mergesort/Graphs/part4_graphs/Random/main_Variance_Random_268435456.png)

**Merge Sort Comm Graphs**
![Comm Random Input Size 65536](./mergesort/Graphs/part4_graphs/Random/comm_Random_inputsize_65536.png)
![Comm Random Input Size 268435456](./mergesort/Graphs/part4_graphs/Random/comm_Random_inputsize_268435456.png)
![Comm Variance Random 65536](./mergesort/Graphs/part4_graphs/Random/comm_Variance_Random_65536.png)
![Comm Variance Random 268435456](./mergesort/Graphs/part4_graphs/Random/comm_Variance_Random_268435456.png)

**Merge Sort Comp Large Graphs**
![Comp Large Random Input Size 65536](./mergesort/Graphs/part4_graphs/Random/comp_large_Random_inputsize_65536.png)
![Comp Large Random Input Size 268435456](./mergesort/Graphs/part4_graphs/Random/comp_large_Random_inputsize_268435456.png)
![Comp Large Variance Random 65536](./mergesort/Graphs/part4_graphs/Random/comp_large_Variance_Random_65536.png)
![Comp Large Variance Random 268435456](./mergesort/Graphs/part4_graphs/Random/comp_large_Variance_Random_268435456.png)

**Analysis**

For my graphs, you can see that as you increase the processors on the smaller array sizes, the time per each process actually increases. This shows that the overhead when computing these smaller sizes costs more than it would to merge on a single process. The cost of parallelizing at these sizes is not worth it. However, as you get to the larger input sizes, as you increase the number of processes, you decrease the overall time. This is the point of having our parallelized merge sort. This proves that if the input size is big enough, then the cost of parallelizing the algorithm is worth it, as it is less than the cost it would take to compute on less processors. For every graph, each data type is actually very similar, with the most difference between each type being in the 16777216 graph. There are some spikes in the graphs, but I believe these are due to noise. It makes sense that for the larger graphs, when parallelizing them, the random inputs have a higher time because part of the algorithm involves comparing the elements and the elements being randomly sorted causes more comparisons. For the communication graphs, it is evident that there is a slight downward trend. The trend is not that sever, but it illustrates that as you increase the number of processes, there is less data being transfered between each process and thus the comm_time for each process is lower. As for the comp_large graphs, it is evident that even at smaller and larger inputs, as you increase the number of processes, the comp_large time decreases. This makes sense because there are more processes to split up the work, making it easier for each process to compute their results and merge them. As for the variance, you can see that there is definitely more variance in the comm graphs, and this makes sense because we are running our algorithms on HPRC Grace, meaning that we are more prone to network traffic and noisy neighbors on the super computer. This means that we are more likely to run into noise. As for the comp_large graphs, you can see that as we increase the number of processes, the variance decreases. This makes sense because each process will have similar amounts of smaller work, meaning that they will be able to process very quickly and in less time, thus causing less overall variance compared to the lower processes's variances.

**Radix Sort Main Graphs**
![Main Times 65536](./radixsort/main_graphs/main_input_size_65536.png)
![Main Times 262144](./radixsort/main_graphs/main_input_size_262144.png)
![Main Times 1048576](./radixsort/main_graphs/main_input_size_1048576.png)
![Main Times 4194304](./radixsort/main_graphs/main_input_size_4194304.png)
![Main Times 16777216](./radixsort/main_graphs/main_input_size_16777216.png)
![Main Times 67108864](./radixsort/main_graphs/main_input_size_67108864.png)
![Main Times 268435456](./radixsort/main_graphs/main_input_size_268435456.png)
![Main Variance Random 65536](./radixsort/part4_graphs/Random/main_Variance_Random_65536.png)
![Main Variance Random 268435456](./radixsort/part4_graphs/Random/main_Variance_Random_268435456.png)

**Radix Sort Comm Graphs**
![Comm Random Input Size 65536](./radixsort/part4_graphs/Random/comm_Random_inputsize_65536.png)
![Comm Random Input Size 268435456](./radixsort/part4_graphs/Random/comm_Random_inputsize_268435456.png)
![Comm Variance Random 65536](./radixsort/part4_graphs/Random/comm_Variance_Random_65536.png)
![Comm Variance Random 268435456](./radixsort/part4_graphs/Random/comm_Variance_Random_268435456.png)

**Radix Sort Comp Large Graphs**
![Comp Large Random Input Size 65536](./radixsort/part4_graphs/Random/comp_large_Random_inputsize_65536.png)
![Comp Large Random Input Size 268435456](./radixsort/part4_graphs/Random/comp_large_Random_inputsize_268435456.png)
![Comp Large Variance Random 65536](./radixsort/part4_graphs/Random/comp_large_Variance_Random_65536.png)
![Comp Large Variance Random 268435456](./radixsort/part4_graphs/Random/comp_large_Variance_Random_268435456.png)

**Analysis**

After parallelizing the merging of local arrays, this implementation of radix sort now better reflects the benefits of increasing process count for larger input sizes. With lower input sizes, the runtime of main unfortunately increases with more processes as the increased communication time between so many processes outweighs the benefit from parallelism. As the input size grows, we see that parallelism helps decrease runtime significantly, albeit with diminishing returns. Massive spikes in communication time can be seen at 512 processes on the two largest input sizes - this can be attributed to inconsistency between nodes used, since this spike is not consistent between input types and is seen to a lower degree on lower input sizes as well. All computation graphs show the same type of downwards curve, reflecting the benefit of parallelism in those areas and the decreased gain in performance as more and more processes are used. In general, the communication graphs show higher variance on the lower input sizes due to the runtime being so low that any fluctuation can dramatically affect the graph. We still see the spike on the highest input size correlating to the spike in the main runtime. 

## 5. Presentation
Plots for the presentation should be as follows:
- For each implementation:
    - For each of comp_large, comm, and main:
        - Strong scaling plots for each input_size with lines for input_type (7 plots - 4 lines each)
        - Strong scaling speedup plot for each input_type (4 plots)
        - Weak scaling plots for each input_type (4 plots)

Analyze these plots and choose a subset to present and explain in your presentation.

**Perturbed Graphs**

![Comm Perc Perturbed 65536](./combined_graphs/1_perc_perturbed/comm/comm_1_perc_perturbed_65536.png)
![Comm Perc Perturbed 268435456](./combined_graphs/1_perc_perturbed/comm/comm_1_perc_perturbed_268435456.png)
![Comp Large Perc Perturbed 65536](./combined_graphs/1_perc_perturbed/comp_large/comp_large_1_perc_perturbed_65536.png)
![Comp Large Perc Perturbed 268435456](./combined_graphs/1_perc_perturbed/comp_large/comp_large_1_perc_perturbed_268435456.png)
![MainPerc Perturbed 65536](./combined_graphs/1_perc_perturbed/main/main_1_perc_perturbed_65536.png)
![MainPerc Perturbed 268435456](./combined_graphs/1_perc_perturbed/main/main_1_perc_perturbed_268435456.png)
![Comm Perc Perturbed Speedup 65536](./combined_graphs/1_perc_perturbed/speedup/comm_speedup_1_perc_perturbed_65536.png)
![Comm Perc Perturbed Speedup 268435456](./combined_graphs/1_perc_perturbed/speedup/comm_speedup_1_perc_perturbed_268435456.png)
![Comp Large Perc Perturbed Speedup 65536](./combined_graphs/1_perc_perturbed/speedup/comp_large_speedup_1_perc_perturbed_65536.png)
![Comp Large Perc Perturbed Speedup 268435456](./combined_graphs/1_perc_perturbed/speedup/comp_large_speedup_1_perc_perturbed_268435456.png)
![Main Perc Perturbed Speedup 65536](./combined_graphs/1_perc_perturbed/speedup/main_speedup_1_perc_perturbed_65536.png)
![Main Perc Perturbed Speedup 268435456](./combined_graphs/1_perc_perturbed/speedup/main_speedup_1_perc_perturbed_268435456.png)

**Random Graphs**
![Comm Random 65536](./combined_graphs/Random/comm/comm_Random_65536.png)
![Comm Random 268435456](./combined_graphs/Random/comm/comm_Random_268435456.png)
![Comp Large Random 65536](./combined_graphs/Random/comp_large/comp_large_Random_65536.png)
![Comp Large Random 268435456](./combined_graphs/Random/comp_large/comp_large_Random_268435456.png)
![Main Random 65536](./combined_graphs/Random/main/main_Random_65536.png)
![Main Random 268435456](./combined_graphs/Random/main/main_Random_268435456.png)
![Comm Random Speedup 65536](./combined_graphs/Random/speedup/comm_speedup_Random_65536.png)
![Comm Random Speedup 268435456](./combined_graphs/Random/speedup/comm_speedup_Random_268435456.png)
![Comp Large Random Speedup 65536](./combined_graphs/Random/speedup/comp_large_speedup_Random_65536.png)
![Comp Large Random Speedup 268435456](./combined_graphs/Random/speedup/comp_large_speedup_Random_268435456.png)
![Main Random Speedup 65536](./combined_graphs/Random/speedup/main_speedup_Random_65536.png)
![Main Random Speedup 268435456](./combined_graphs/Random/speedup/main_speedup_Random_268435456.png)

**Reverse Sorted Graphs**
![Comm ReverseSorted 65536](./combined_graphs/ReverseSorted/comm/comm_ReverseSorted_65536.png)
![Comm ReverseSorted 268435456](./combined_graphs/ReverseSorted/comm/comm_ReverseSorted_268435456.png)
![Comp Large ReverseSorted 65536](./combined_graphs/ReverseSorted/comp_large/comp_large_ReverseSorted_65536.png)
![Comp Large ReverseSorted 268435456](./combined_graphs/ReverseSorted/comp_large/comp_large_ReverseSorted_268435456.png)
![Main ReverseSorted 65536](./combined_graphs/ReverseSorted/main/main_ReverseSorted_65536.png)
![Main ReverseSorted 268435456](./combined_graphs/ReverseSorted/main/main_ReverseSorted_268435456.png)
![Comm ReverseSorted Speedup 65536](./combined_graphs/ReverseSorted/speedup/comm_speedup_ReverseSorted_65536.png)
![Comm ReverseSorted Speedup 268435456](./combined_graphs/ReverseSorted/speedup/comm_speedup_ReverseSorted_268435456.png)
![Comp Large ReverseSorted Speedup 65536](./combined_graphs/ReverseSorted/speedup/comp_large_speedup_ReverseSorted_65536.png)
![Comp Large ReverseSorted Speedup 268435456](./combined_graphs/ReverseSorted/speedup/comp_large_speedup_ReverseSorted_268435456.png)
![Main ReverseSorted Speedup 65536](./combined_graphs/ReverseSorted/speedup/main_speedup_ReverseSorted_65536.png)
![Main ReverseSorted Speedup 268435456](./combined_graphs/ReverseSorted/speedup/main_speedup_ReverseSorted_268435456.png)

**Sorted Graphs**
![Comm Sorted 65536](./combined_graphs/Sorted/comm/comm_Sorted_65536.png)
![Comm Sorted 268435456](./combined_graphs/Sorted/comm/comm_Sorted_268435456.png)
![Comp Large Sorted 65536](./combined_graphs/Sorted/comp_large/comp_large_Sorted_65536.png)
![Comp Large Sorted 268435456](./combined_graphs/Sorted/comp_large/comp_large_Sorted_268435456.png)
![Main Sorted 65536](./combined_graphs/Sorted/main/main_Sorted_65536.png)
![Main Sorted 268435456](./combined_graphs/Sorted/main/main_Sorted_268435456.png)
![Comm Sorted Speedup 65536](./combined_graphs/Sorted/speedup/comm_speedup_Sorted_65536.png)
![Comm Sorted Speedup 268435456](./combined_graphs/Sorted/speedup/comm_speedup_Sorted_268435456.png)
![Comp Large Sorted Speedup 65536](./combined_graphs/Sorted/speedup/comp_large_speedup_Sorted_65536.png)
![Comp Large Sorted Speedup 268435456](./combined_graphs/Sorted/speedup/comp_large_speedup_Sorted_268435456.png)
![Main Sorted Speedup 65536](./combined_graphs/Sorted/speedup/main_speedup_Sorted_65536.png)
![Main Sorted Speedup 268435456](./combined_graphs/Sorted/speedup/main_speedup_Sorted_268435456.png)

**1_perc_perturbed analysis**

For the comm graphs, you can see that the merge and radix sort are similar and the sample sort and bitonic sort are similar. This is true for both sizes. As you can see for both comp_large graphs, the average time per ranks for each graph decreases as you increase the number of processes. It is interesting because sample sort starts off at a way higher average time compared to every other sorting alogrithm, with merge and radix sort being the most similar. For the main grapahs, you can see that every single algorithm increases in time for the lowest array size. This shows that for every single sorting algorithm, it is not ever worth it to parallelize at smaller array sizes due to the overhead parallelizing the algorithm causes. However, for the biggest array, every algorithm decreases in time, meaning that it is worth it. Interestingly, merge sort and radix sort for the smaller size are quite different, but for the largest size, they are amlost identical. As well, you can see that sample sort has the biggest difference in time again. In all of our speed up graphs, you can see that these findings are visualized. It is quite crazy to see the differences in the speed up between sample sort and merge sort. Merge sort sped up the least compared to every other sorting algorithm, followed by radix sort, then bitonic sort, and finally sample sort. Sample sort was almost 100x faster and merge sort was less than 20 times faster when comparing at the max size and the least processes compared to the most processes. 

**Sorted analysis**
As you can see, the communication graphs seem quite random but seme to spike when we add more processes. This can make sense since we're communicating on more nodes, thus allowing for more variance (also because Grace noise). Regarding large computations, it seems like for both small and large array sizes, it follows the same trend for all algorithms. It follows an exponential decay pattern in the order of magnitude of sample sort, radix sort, bitonic sort, and merge sort. For the main graphs, it seems mostly the same order with sample sort notably having a higher magnitude than the rest of the algorithms. You can actually see, however, for main graphs, that with a smaller array size our total time increases as we add more processes due to overhead and MPI. As for the comm speedup, it seems like a mostly random graph yet again except for larger array sizes, the total speedup increases by a large magnitude as we add more processes. For the speedup of large computations, it seems to follow the trend of an exponential increase for all implementations with the highest increase going to merge sort, followed by radix sort, followed by sample sort, followed by bitonic sort for small and large array sizes. For main speedup, for large array sizes, it seems like sample sort has the largest speedup followed by radix/bitonic and finally merge sort.


## 6. Final Report
Submit a zip named `TeamX.zip` where `X` is your team number. The zip should contain the following files:
- Algorithms: Directory of source code of your algorithms.
- Data: All `.cali` files used to generate the plots seperated by algorithm/implementation.
- Jupyter notebook: The Jupyter notebook(s) used to generate the plots for the report.
- Report.md
