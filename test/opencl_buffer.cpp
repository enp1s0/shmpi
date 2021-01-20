#include <iostream>
#include <vector>
#include <memory>
#include <shmpi/shmpi.hpp>
#include <shmpi/opencl_buffer.hpp>

constexpr std::size_t N = 1lu << 27;
constexpr std::size_t buffer_count = 1lu << 25;

int main(int argc, char** argv) {
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	const auto& platform = platforms[0];

	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_DEFAULT, &devices);
	auto cl_device = devices[0];

	auto cl_context = cl::Context(cl_device);
	auto cl_queue = cl::CommandQueue(cl_context, cl_device, CL_QUEUE_PROFILING_ENABLE);

	MPI_Init(&argc, &argv);

	int rank, nprocs;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	if (rank == 0) {
		std::printf("-----\n");
		std::printf("# test   : %s / %s\n", __FILE__, __func__);
		std::printf("# N      : %lu\n", N);
		std::printf("# Buffer : %lu\n", buffer_count);
	}

	// allocate test array
	auto test_array = std::unique_ptr<double>(new double[N]);
	cl::Buffer cl_buffer = cl::Buffer(cl_context, CL_MEM_READ_WRITE, sizeof(double) * N);

	// initialize test array
	for (std::size_t i = 0; i < N; i++) {
		test_array.get()[i] = rank + 1;
	}

	cl_queue.enqueueWriteBuffer(cl_buffer, true, 0, N * sizeof(double), test_array.get());
	cl_queue.finish();

	// initialize shmpi buffer
	shmpi::opencl_buffer<double> buffer(buffer_count, cl_queue);
	buffer.allocate();
	buffer.set_cl_buffer(&cl_buffer);

	// allreduce
	std::printf("[%3d/%3d] : Start Allreduce\n", rank, nprocs);
	shmpi::shmpi_allreduce(
			shmpi::shmpi_in_place,
			0,
			&buffer,
			0,
			N,
			MPI_DOUBLE,
			MPI_SUM,
			MPI_COMM_WORLD
			);
	std::printf("[%3d/%3d] : Allreduce Done\n", rank, nprocs);

	cl_queue.enqueueReadBuffer(cl_buffer, true, 0, N * sizeof(double), test_array.get());
	cl_queue.finish();

	// validate result array
	double error =0.;
	const auto correct = 0.5 * nprocs * (nprocs + 1);
	for (std::size_t i = 0; i < N; i++) {
		const auto diff = correct - test_array.get()[i];
		error = std::max(std::abs(diff), error);
	}
	std::printf("[%3d/%3d] : error = %e\n", rank, nprocs, error);
	MPI_Barrier(MPI_COMM_WORLD);

	MPI_Finalize();
}
