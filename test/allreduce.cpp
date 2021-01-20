#include <iostream>
#include <memory>
#include <cmath>
#include <shmpi/shmpi.hpp>
#include "cpu_buffer.hpp"

constexpr std::size_t N = 1lu << 25;
constexpr std::size_t buffer_count = 1lu << 20;

void allreduce_test_0(const int rank, const int nprocs) {
	if (rank == 0) {
		std::printf("# test   : %s / %s\n", __FILE__, __func__);
		std::printf("# N      : %lu\n", N);
		std::printf("# Buffer : %lu\n", buffer_count);
	}
	std::unique_ptr<double[]> test_array_send(new double[N]);
	std::unique_ptr<double[]> test_array_recv(new double[N]);

	mtk::test::cpu_buffer<double> send_buffer(buffer_count);
	mtk::test::cpu_buffer<double> recv_buffer(buffer_count);
	send_buffer.allocate();
	send_buffer.set_org_ptr(test_array_send.get());
	recv_buffer.allocate();
	recv_buffer.set_org_ptr(test_array_recv.get());

	// Init test array
	for (std::size_t i = 0; i < N; i++) {
		test_array_send.get()[i] = rank + 1;
	}

	// Call allreduce
	std::printf("[%3d/%3d] : Start Allreduce\n", rank, nprocs);
	shmpi::shmpi_allreduce(&send_buffer, 0, &recv_buffer, 0, N, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	std::printf("[%3d/%3d] : Allreduce Done\n", rank, nprocs);

	// Validate result array
	double error = 0.;
	const auto correct = 0.5 * nprocs * (nprocs + 1);
	for (std::size_t i = 0; i < N; i++) {
		const auto diff = test_array_recv.get()[i] - correct;
		error = std::max(std::abs(diff), error);
	}
	std::printf("[%3d/%3d] : error = %e\n", rank, nprocs, error);
}

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, nprocs;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	allreduce_test_0(rank, nprocs);

	MPI_Finalize();
}
