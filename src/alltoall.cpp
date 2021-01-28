#include <thread>
#include <shmpi/shmpi.hpp>
#include <cassert>

int shmpi::shmpi_alltoall(
		shmpi::buffer* const send_buffer,
		const std::size_t offset_send,
		const std::size_t send_count,
		MPI_Datatype send_type,
		shmpi::buffer* const recv_buffer,
		const std::size_t offset_recv,
		const std::size_t recv_count,
		MPI_Datatype recv_type,
		MPI_Comm comm
		) {
	assert(send_type == recv_type);
	assert(send_count == recv_count);

	if (send_buffer == shmpi::shmpi_in_place) {
		return shmpi::shmpi_alltoall(
			recv_buffer, offset_recv, recv_count,recv_type,
			recv_buffer, offset_recv, recv_count,recv_type,
			comm
			);
	}

	const bool send_recv_same = send_buffer == recv_buffer;

	int nprocs;
	MPI_Comm_size(comm, &nprocs);

	int rank;
	MPI_Comm_rank(comm, &rank);

	const auto buffer_count = (send_buffer == shmpi::shmpi_in_place) ? recv_buffer->get_buffer_count() : std::min(send_buffer->get_buffer_count(), recv_buffer->get_buffer_count());
	const auto data_count = send_count * nprocs;

	if (buffer_count >= data_count) {
		send_buffer->read_from_device(0, 0, data_count);

		const auto stat = MPI_Alltoall(
			send_buffer->get_ptr(0), send_count, send_type,
			recv_buffer->get_ptr(1), recv_count, recv_type,
			comm);

		if (stat != MPI_SUCCESS) {
			return stat;
		}

		recv_buffer->write_to_device(1, 0, data_count);

		return MPI_SUCCESS;
	}

	const auto buffer_count_per_rank = buffer_count / nprocs;
	for (int r = 0; r < nprocs; r++) {
		send_buffer->read_from_device(0, r * send_count + 0 * buffer_count_per_rank, buffer_count_per_rank, r * buffer_count_per_rank);
	}
	std::size_t b = 0;
	for (; b < send_count / buffer_count_per_rank - 1; b++) {
		// Read next buffer
		const auto read_next_buffer_func = [=]() {
			for (int r = 0; r < nprocs; r++) {
				const auto real_data_count = std::min(buffer_count_per_rank, send_count - (b + 1) * buffer_count_per_rank);
				send_buffer->read_from_device(1 - (b % 2), r * send_count + (b + 1) * buffer_count_per_rank, real_data_count, r * buffer_count_per_rank);
			}
		};
		std::thread read_next_buffer_thread(read_next_buffer_func);

		// Alltoall buffer data
		void *send_buffer_ptr = send_buffer->get_ptr(b % 2);
		if (send_recv_same) {
			send_buffer_ptr = MPI_IN_PLACE;
		}
		const auto stat = MPI_Alltoall(
			send_buffer_ptr,
			buffer_count_per_rank,
			send_type,
			recv_buffer->get_ptr(b % 2),
			buffer_count_per_rank,
			recv_type,
			comm
			);
		if (stat != MPI_SUCCESS) {
			return stat;
		}

		// Write to device
		for (int r = 0; r < nprocs; r++) {
			const auto real_data_count = std::min(buffer_count_per_rank, recv_count - b * buffer_count_per_rank);
			recv_buffer->write_to_device(b % 2, r * recv_count + b * buffer_count_per_rank, real_data_count, r * buffer_count_per_rank);
		}

		// wait read buffer
		read_next_buffer_thread.join();
	}

	// Alltoall buffer data
	void *send_buffer_ptr = send_buffer->get_ptr(b % 2);
	if (send_recv_same) {
		send_buffer_ptr = MPI_IN_PLACE;
	}
	const auto stat = MPI_Alltoall(
		send_buffer_ptr,
		buffer_count_per_rank,
		send_type,
		recv_buffer->get_ptr(b % 2),
		buffer_count_per_rank,
		recv_type,
		comm
		);
	if (stat != MPI_SUCCESS) {
		return stat;
	}

	// Write to device
	for (int r = 0; r < nprocs; r++) {
		const auto real_data_count = std::min(buffer_count_per_rank, send_count - b * buffer_count_per_rank);
		recv_buffer->write_to_device(b % 2, r * recv_count + b * buffer_count_per_rank, real_data_count, r * buffer_count_per_rank);
	}

	return MPI_SUCCESS;
}
