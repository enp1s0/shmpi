#include <iostream>
#include <memory>
#include <cmath>
#include <shmpi/shmpi.hpp>
#include "cpu_buffer.hpp"

constexpr std::size_t N = 1lu << 25;
constexpr std::size_t buffer_count = 1lu << 15;

void alltoall_test_0(const std::size_t N, const std::size_t buffer_count, const int rank, const int nprocs) {
	if (rank == 0) {
		std::printf("-----\n");
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

	// Call alltoall
	std::printf("[%3d/%3d] : Start Allreduce\n", rank, nprocs);
	const auto stat = shmpi::shmpi_alltoall(&send_buffer, 0, N / nprocs, MPI_DOUBLE, &recv_buffer, 0, N / nprocs, MPI_DOUBLE, MPI_COMM_WORLD);
	std::printf("[%3d/%3d] : Allreduce Done {stat = %d}\n", rank, nprocs, stat);

	// Validate result array
	double error = 0.;
	for (std::size_t i = 0; i < N; i++) {
		const std::size_t correct = i / (N / nprocs) + 1;
		const auto diff = test_array_recv.get()[i] - correct;
		error = std::max(std::abs(diff), error);
	}
	std::printf("[%3d/%3d] : error = %e\n", rank, nprocs, error);
	MPI_Barrier(MPI_COMM_WORLD);
}

void alltoall_test_1_in_place(const std::size_t N, const std::size_t buffer_count, const int rank, const int nprocs) {
	if (rank == 0) {
		std::printf("-----\n");
		std::printf("# test   : %s / %s\n", __FILE__, __func__);
		std::printf("# N      : %lu\n", N);
		std::printf("# Buffer : %lu\n", buffer_count);
	}
	std::unique_ptr<double[]> test_array(new double[N]);

	mtk::test::cpu_buffer<double> buffer(buffer_count);
	buffer.allocate();
	buffer.set_org_ptr(test_array.get());

	// Init test array
	for (std::size_t i = 0; i < N; i++) {
		test_array.get()[i] = rank + 1;
	}

	// Call alltoall
	std::printf("[%3d/%3d] : Start Allreduce\n", rank, nprocs);
	shmpi::shmpi_alltoall(shmpi::shmpi_in_place, 0, N / nprocs, MPI_DOUBLE, &buffer, 0, N / nprocs, MPI_DOUBLE, MPI_COMM_WORLD);
	std::printf("[%3d/%3d] : Allreduce Done\n", rank, nprocs);

	// Validate result array
	double error = 0.;
	for (std::size_t i = 0; i < N; i++) {
		const std::size_t correct = i / (N / nprocs) + 1;
		const auto diff = test_array.get()[i] - correct;
		error = std::max(std::abs(diff), error);
	}
	std::printf("[%3d/%3d] : error = %e\n", rank, nprocs, error);
	MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, nprocs;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	alltoall_test_0(N				, buffer_count, rank, nprocs);
	alltoall_test_0(N + N / 2		, buffer_count, rank, nprocs);
	alltoall_test_0(buffer_count / 2, buffer_count, rank, nprocs);
	alltoall_test_1_in_place(N					, buffer_count, rank, nprocs);
	alltoall_test_1_in_place(N + N / 2			, buffer_count, rank, nprocs);
	alltoall_test_1_in_place(buffer_count / 2	, buffer_count, rank, nprocs);

	MPI_Finalize();
}
