#include <iostream>
#include <cmath>
#include <memory>
#include <shmpi/shmpi.hpp>
#include "cpu_buffer.hpp"

constexpr std::size_t N = 1lu << 25;
constexpr std::size_t buffer_size = 1lu << 15;

void test_0(const std::size_t N, const std::size_t buffer_size, const int rank, const int nprocs) {
	if (rank == 0) {
		std::printf("# test   : %s / %s\n", __FILE__, __func__);
		std::printf("# N      : %lu\n", N);
		std::printf("# Buffer : %lu\n", buffer_size);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	std::printf("[%d/%d]: Allocating test array\n", rank, nprocs);
	std::unique_ptr<double[]> send_array(new double [N]);
	std::unique_ptr<double[]> recv_array(new double [N]);

	std::printf("[%d/%d]: Allocating shmpi buffer\n", rank, nprocs);
	mtk::test::cpu_buffer<double> send_buffer(buffer_size);
	mtk::test::cpu_buffer<double> recv_buffer(buffer_size);
	send_buffer.allocate();
	recv_buffer.allocate();
	send_buffer.set_org_ptr(send_array.get());
	recv_buffer.set_org_ptr(recv_array.get());

	std::printf("[%d/%d]: Initialize test array\n", rank, nprocs);
	for (std::size_t i = 0; i < N; i++) {
		send_array.get()[i] = rank;
	}

	std::printf("[%d/%d]: Call sendrecv\n", rank, nprocs);
	const auto stat = shmpi::shmpi_sendrecv(
		&send_buffer,
		0,
		N,
		MPI_DOUBLE,
		(rank + 1) % nprocs,
		0,
		&recv_buffer,
		0,
		N,
		MPI_DOUBLE,
		(nprocs + rank - 1) % nprocs,
		0,
		MPI_COMM_WORLD
		);
	std::printf("[%d/%d]: sendrecv done {stat = %d}\n", rank, nprocs, stat);

	double max_error = 0.;
	const auto correct = static_cast<double>((nprocs + rank - 1) % nprocs);
	for (std::size_t i = 0; i < N; i++) {
		const auto diff = recv_array.get()[i] - correct;
		max_error = std::max(max_error, diff);
	}
	std::printf("[%03d/%03d] max_error = %e\n", rank, nprocs, max_error);
	MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, nprocs;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	test_0(buffer_size / 2	, buffer_size, rank, nprocs);
	test_0(N				, buffer_size, rank, nprocs);
	test_0(N + N / 2		, buffer_size, rank, nprocs);

	MPI_Finalize();
}
