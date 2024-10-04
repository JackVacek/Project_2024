# CSCE 435 Group project

## 0. Group number: 1 

### **Communicate through Discord**

## 1. Group members:
1. Jack Vacek - Merge Sort
2. Christopher Vu - Bitonic Sort
3. Third
4. Zak Borman - Radix Sort

## 2. Project topic: Parallel Sorting Algorithms

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
- Merge Sort:
- Radix Sort: A linear sorting algorithm that sorts elements by processing them digit by digit. Rather than comparing elements directly, Radix Sort distributes the elements into buckets based on each digit’s value. By repeatedly sorting the elements by their significant digits, from the least significant to the most significant, Radix Sort achieves the final sorted order. 

### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes

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
