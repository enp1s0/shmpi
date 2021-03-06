#include <iostream>
#include <cmath>
#include <memory>
#include <shmpi/shmpi.hpp>
#include "cpu_buffer.hpp"

constexpr std::size_t N = 1lu << 30;
constexpr std::size_t buffer_size = 1lu << 20;

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, nprocs;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	if (rank == 0) {
		std::printf("# test   : %s\n", __FILE__);
		std::printf("# N      : %lu\n", N);
		std::printf("# Buffer : %lu\n", buffer_size);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	std::printf("[%d/%d]: Allocating test array\n", rank, nprocs);
	std::unique_ptr<double[]> test_array(new double [N]);

	std::printf("[%d/%d]: Allocating shmpi buffer\n", rank, nprocs);
	mtk::test::cpu_buffer<double> buffer(buffer_size);
	buffer.allocate();
	buffer.set_org_ptr(test_array.get());

	if (rank == 0) {
		std::printf("[%d/%d]: Initialize test array values\n", rank, nprocs);
		for (std::size_t i = 0; i < N; i++) {
			test_array.get()[i] = i;
		}
		std::printf("[%d/%d]: Start SEND\n", rank, nprocs);
		shmpi::shmpi_send(&buffer, 0, N, MPI_UINT64_T, 1, 0, MPI_COMM_WORLD);
		std::printf("[%d/%d]: SEND Done\n", rank, nprocs);
	} else {
		std::printf("[%d/%d]: Start RECV\n", rank, nprocs);
		shmpi::shmpi_recv(&buffer, 0, N, MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
		std::printf("[%d/%d]: RECV Done\n", rank, nprocs);
		std::printf("[%d/%d]: Validate test array values\n", rank, nprocs);
		double error = 0.0;
		for (std::size_t i = 0; i < N; i++) {
			const auto diff = test_array.get()[i] - static_cast<double>(i);
			error = std::max(std::abs(diff), error);
		}
		std::printf("[%d/%d]: max_error = %e\n", rank, nprocs, error);
	}

	MPI_Finalize();
}
