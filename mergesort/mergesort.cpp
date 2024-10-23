#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctime>
#include <algorithm>
#include <random>
#include <string>
#include <iostream>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0      /* taskid of first task */
#define FROM_MASTER 1 /* setting a message type */
#define FROM_WORKER 2 /* setting a message type */

void choose_array_type(int *a, std::string type, int sizeOfArray)
{
    // std::cout << "The array type is: " << type << std::endl;
    if (type == "Random")
    {
        // Random
        std::mt19937 gen(static_cast<unsigned int>(std::time(0)));
        std::uniform_int_distribution<> dist(1, sizeOfArray);

        for (int i = 0; i < sizeOfArray; ++i)
        {
            a[i] = dist(gen);
        }
    }
    if (type == "ReverseSorted")
    {
        // std::cout << "is reverse sorted" << std::endl;
        //  reverse sorted
        int index = 0;
        for (int i = sizeOfArray - 1; i >= 0; --i)
        {
            a[index] = i + 1;
            index += 1;
        }
    }

    if (type == "1_perc_perturbed")
    {
        // 1% perturbed
        std::mt19937 gen(static_cast<unsigned int>(std::time(0)));
        std::uniform_int_distribution<> dist(1, sizeOfArray);
        std::uniform_int_distribution<> perturb(0, 99);

        for (int i = 0; i < sizeOfArray; ++i)
        {
            if (perturb(gen) == 0)
            {
                a[i] = dist(gen);
            }
            else
            {
                a[i] = i + 1;
            }
        }
    }
    if (type == "Sorted")
    {
        // sorted
        for (int i = 0; i < sizeOfArray; i++)
        {
            a[i] = i + 1;
        }
    }

    // std::cout << "printing array" << std::endl;
    // for (int i = 0; i < sizeOfArray; ++i)
    //{
    //     std::cout << a[i] << " ";
    // }
    // std::cout << std::endl;
}

bool correctness_checker(int *global, int sizeOfArray)
{
    // std::cout << "Checking to see if the program is correct" << std::endl;
    //  calculate if the program is correct
    for (int i = 0; i < sizeOfArray - 1; i++)
    {
        if (global[i] > global[i + 1] || global[i] < 1 || global[i] > sizeOfArray || global[i + 1] < 1 || global[i + 1] > sizeOfArray)
        {
            // printf("The program is incorrect\n");
            return false;
        }
        // std::cout << global[i] << " ";
    }
    // std::cout << global[sizeOfArray - 1] << std::endl;
    return true;
}

void merge(int *arr, int left, int mid, int right)
{
    int endFirst = mid - left + 1;
    int endSecond = right - mid;

    int *firstHalf = new int[endFirst];
    int *secondHalf = new int[endSecond];

    for (int i = 0; i < endFirst; i++)
    {
        firstHalf[i] = arr[left + i];
    }
    for (int i = 0; i < endSecond; i++)
    {
        secondHalf[i] = arr[mid + 1 + i];
    }

    int i = 0;
    int j = 0;
    int k = left;

    while (i < endFirst && j < endSecond)
    {
        if (firstHalf[i] <= secondHalf[j])
        {
            arr[k] = firstHalf[i];
            i++;
        }
        else
        {
            arr[k] = secondHalf[j];
            j++;
        }
        k++;
    }

    while (i < endFirst)
    {
        arr[k] = firstHalf[i];
        i++;
        k++;
    }

    while (j < endSecond)
    {
        arr[k] = secondHalf[j];
        j++;
        k++;
    }
    delete[] firstHalf;
    delete[] secondHalf;
}

void mergeSort(int *arr, int left, int right)
{
    if (left >= right)
    {
        return;
    }
    int mid = left + (right - left) / 2;
    mergeSort(arr, left, mid);
    mergeSort(arr, mid + 1, right);
    merge(arr, left, mid, right);
}

int main(int argc, char *argv[])
{
    CALI_CXX_MARK_FUNCTION;

    int sizeOfArray;
    std::string typeOfArray;
    if (argc == 3)
    {
        sizeOfArray = atoi(argv[1]);
        typeOfArray = argv[2];
        // std::cout << typeOfArray << std::endl;
        // std::cout << sizeOfArray << std::endl;
    }
    else
    {
        printf("\n Please provide the size of the array and type of array as an argument. \n0 = random \n1 = sorted \n2 = reverse sorted \n3 = 1 percent perturbed \n");
        return 0;
    }
    int numtasks, /* number of tasks in partition */
        taskid,   /* a task identifier */
        rc;       /* return code */

    /* Define Caliper region names */
    const char *data_init_runtime = "data_init_runtime";
    const char *correctness_check = "correctness_check";
    const char *comm = "comm";
    const char *comm_small = "comm_small";
    const char *comm_large = "comm_large";
    const char *comp = "comp";
    const char *comp_small = "comp_small";
    const char *comp_large = "comp_large";

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);   // get process id
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // get number of processes

    int *a = new int[sizeOfArray];
    int localArraySize = sizeOfArray / numtasks;
    int *localArrays = new int[localArraySize];
    // std::cout << localArraySize << std::endl;
    cali::ConfigManager mgr;
    mgr.start();

    if (numtasks < 2)
    {
        printf("Need at least two MPI tasks. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }

    if (taskid == 0)
    {
        CALI_MARK_BEGIN(data_init_runtime);
        choose_array_type(a, typeOfArray, sizeOfArray);
        CALI_MARK_END(data_init_runtime);
        // for (int i = 0; i < sizeOfArray; ++i)
        //{
        //    std::cout << a[i] << " ";
        //}
        // std::cout << std::endl;
    }
    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Scatter(a, localArraySize, MPI_INT, localArrays, localArraySize, MPI_INT, MASTER, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    CALI_MARK_BEGIN(comp);
    CALI_MARK_BEGIN(comp_small);
    mergeSort(localArrays, 0, localArraySize - 1);
    CALI_MARK_END(comp_small);
    CALI_MARK_END(comp);

    CALI_MARK_BEGIN(comm);
    CALI_MARK_BEGIN(comm_large);
    MPI_Gather(localArrays, localArraySize, MPI_INT, a, localArraySize, MPI_INT, MASTER, MPI_COMM_WORLD);
    CALI_MARK_END(comm_large);
    CALI_MARK_END(comm);

    if (taskid == 0)
    {
        CALI_MARK_BEGIN(comp);
        CALI_MARK_BEGIN(comp_large);
        for (int i = 1; i < numtasks; i++)
        {
            merge(a, 0, (i * localArraySize) - 1, (i + 1) * localArraySize - 1);
        }
        CALI_MARK_END(comp_large);
        CALI_MARK_END(comp);
        bool isCorrect = false;
        CALI_MARK_BEGIN(correctness_check);
        isCorrect = correctness_checker(a, sizeOfArray);
        CALI_MARK_END(correctness_check);
        if (isCorrect)
        {
            printf("The program is correct\n");
        }
        else
        {
            printf("The program is incorrect\n");
        }
        // for (int i = 0; i < sizeOfArray; ++i)
        //{
        //     std::cout << a[i] << " ";
        // }
        // std::cout << std::endl;
    }

    mgr.stop();
    mgr.flush();

    adiak::init(NULL);
    adiak::launchdate();                                // launch date of the job
    adiak::libraries();                                 // Libraries used
    adiak::cmdline();                                   // Command line used to launch the job
    adiak::clustername();                               // Name of the cluster
    adiak::value("algorithm", "merge");                 // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi");           // e.g. "mpi"
    adiak::value("data_type", "int");                   // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(int));     // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", sizeOfArray);            // The number of elements in input dataset (1000)
    adiak::value("input_type", typeOfArray);            // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", numtasks);                // The number of processors (MPI ranks)
    adiak::value("scalability", "weak");                // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", 1);                       // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "online/ai/handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    delete[] a;
    delete[] localArrays;
    MPI_Finalize();
}
