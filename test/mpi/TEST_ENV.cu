#include "../include/macros.h"
#include "ErrorChecker.h"
#include "TEST_ENV.h"
#include "mpi.h"
#include <iostream>
int TEST_ENV::gpu_count = 0;
int TEST_ENV::mpi_size = 0;
int TEST_ENV::mpi_rank = 0;
void TEST_ENV::SetUp() {
    MPI_Init(0, NULL); // seg fault, if pass argc & argv.
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    if (mpi_root != mpi_rank) {
        // only the root rank print logs.
        auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
        auto* default_printer = listeners.default_result_printer();
        listeners.Release(default_printer);
        delete default_printer;
        default_printer = NULL;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MCUDA_ASSERT(cudaGetDeviceCount(&gpu_count));
    MINT_ASSERT(true, gpu_count >= mpi_size);
    // set device
    // each mpi process have to use different device, or the gpu will be
    // crashed.
    MCUDA_ASSERT(cudaSetDevice(TEST_ENV::mpi_rank));
    MPI_Barrier(MPI_COMM_WORLD);
};
void TEST_ENV::TearDown() {
    MPI_Finalize();
};
