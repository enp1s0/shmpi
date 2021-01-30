#include <cassert>
#include <thread>
#include <shmpi/shmpi.hpp>

int shmpi::shmpi_send (
		shmpi::buffer* const buffer,
		const std::size_t offset_arg,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int dest,
		const int tag,
		MPI_Comm comm
		) {
	const auto buffer_count = buffer->get_buffer_count();
	if (count <= buffer_count) {
		// In this case, this function uses buffer 0
		buffer->read_from_device(0, offset_arg, count);
		return MPI_Send(buffer->get_ptr(0), count, data_type, dest, tag, comm);
	}

	auto offset = 0 * buffer_count;
	auto real_buffer_size = std::min(count - offset, buffer_count);
	buffer->read_from_device(0, offset + offset_arg, real_buffer_size);
	std::size_t b = 1;
	for (; b < count / buffer_count; b++) {
		MPI_Request request = MPI_REQUEST_NULL;
		const auto stat = MPI_Isend(buffer->get_ptr(1 - (b % 2)), real_buffer_size, data_type, dest, tag, comm, &request);
		if (stat != MPI_SUCCESS) {
			return stat;
		}

		offset = b * buffer_count;
		real_buffer_size = std::min(count - offset, buffer_count);
		buffer->read_from_device(b % 2, offset + offset_arg, real_buffer_size);
		MPI_Wait(&request, MPI_STATUS_IGNORE);
		request = MPI_REQUEST_NULL;
	}
	MPI_Request request = MPI_REQUEST_NULL;
	const auto stat = MPI_Isend(buffer->get_ptr(1 - (b % 2)), real_buffer_size, data_type, dest, tag, comm, &request);
	if (stat != MPI_SUCCESS) {
		return stat;
	}
	return MPI_SUCCESS;
}

int shmpi::shmpi_recv (
		shmpi::buffer* const buffer,
		const std::size_t offset_arg,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int source,
		const int tag,
		MPI_Comm comm
		) {
	const auto buffer_count = buffer->get_buffer_count();
	if (count <= buffer_count) {
		// In this case, this function uses buffer 0
		const auto stat = MPI_Recv(buffer->get_ptr(0), count, data_type, source, tag, comm, MPI_STATUS_IGNORE);
		buffer->write_to_device(0, offset_arg, count);
		return stat;
	}

	MPI_Request request = MPI_REQUEST_NULL;
	auto offset = 0 * buffer_count;
	auto real_buffer_size = std::min(count - offset, buffer_count);
	const auto stat = MPI_Irecv(buffer->get_ptr(0), real_buffer_size, data_type, source, tag, comm, &request);
	std::size_t b = 0;
	for (; b < count / buffer_count - 1; b++) {
		offset = b * buffer_count;
		real_buffer_size = std::min(count - offset, buffer_count);
		MPI_Wait(&request, MPI_STATUS_IGNORE);
		buffer->write_to_device(b % 2, offset + offset_arg, real_buffer_size);
		request = MPI_REQUEST_NULL;

		const auto stat = MPI_Irecv(buffer->get_ptr(1 - (b % 2)), real_buffer_size, data_type, source, tag, comm, &request);
		if (stat != MPI_SUCCESS) {
			return stat;
		}
	}
	offset = b * buffer_count;
	real_buffer_size = std::min(count - offset, buffer_count);
	MPI_Wait(&request, MPI_STATUS_IGNORE);
	buffer->write_to_device(b % 2, offset + offset_arg, real_buffer_size);
	return MPI_SUCCESS;
}

int shmpi::shmpi_sendrecv(
		shmpi::buffer* const send_buffer,
		const std::size_t send_offset,
		const std::size_t send_count,
		const MPI_Datatype send_data_type,
		const int dest,
		const int send_tag,
		shmpi::buffer* const recv_buffer,
		const std::size_t recv_offset,
		const std::size_t recv_count,
		const MPI_Datatype recv_data_type,
		const int source,
		const int recv_tag,
		MPI_Comm comm
		) {
	assert(send_data_type == recv_data_type);
	assert(send_count == recv_count);

	const auto buffer_count = std::min(send_buffer->get_buffer_count(), recv_buffer->get_buffer_count());

	if (send_count <= buffer_count) {
		send_buffer->read_from_device(0, send_offset, send_count);
		const auto stat = MPI_Sendrecv(
			send_buffer->get_ptr(0),
			send_count,
			send_data_type,
			dest,
			send_tag,
			recv_buffer->get_ptr(1),
			recv_count,
			recv_data_type,
			source,
			recv_tag,
			comm,
			MPI_STATUS_IGNORE
			);
		if (stat != MPI_SUCCESS) {
			return stat;
		}
		recv_buffer->write_to_device(1, recv_offset, recv_count);

		return MPI_SUCCESS;
	}

	send_buffer->read_from_device(0, send_offset, buffer_count);
	std::size_t b = 0;
	for (; b < send_count / buffer_count - 1; b++) {
		// Read next buffer
		const auto read_next_buffer_func = [=]() {
			const auto real_buffer_count = std::min(buffer_count, send_count - (b + 1) * buffer_count);
			send_buffer->read_from_device(1 - (b % 2), send_offset + (b + 1) * buffer_count, real_buffer_count);
		};
		std::thread read_next_buffer_thread(read_next_buffer_func);

		// sendrecv
		const auto real_buffer_count = std::min(buffer_count, send_count - b * buffer_count);
		const auto stat = MPI_Sendrecv(
			send_buffer->get_ptr(b % 2),
			real_buffer_count,
			send_data_type,
			dest,
			send_tag,
			recv_buffer->get_ptr(b % 2),
			real_buffer_count,
			recv_data_type,
			source,
			recv_tag,
			comm,
			MPI_STATUS_IGNORE
			);
		if (stat != MPI_SUCCESS) {
			read_next_buffer_thread.join();
			return stat;
		}

		// Write the result to device memory
		recv_buffer->write_to_device(b % 2, recv_offset + b * buffer_count, real_buffer_count);

		// join reading thread
		read_next_buffer_thread.join();
	}
	// sendrecv
	const auto real_buffer_count = std::min(buffer_count, send_count - b * buffer_count);
	const auto stat = MPI_Sendrecv(
		send_buffer->get_ptr(b % 2),
		real_buffer_count,
		send_data_type,
		dest,
		send_tag,
		recv_buffer->get_ptr(b % 2),
		real_buffer_count,
		recv_data_type,
		source,
		recv_tag,
		comm,
		MPI_STATUS_IGNORE
		);
	if (stat != MPI_SUCCESS) {
		return stat;
	}

	// Write the result to device memory
	recv_buffer->write_to_device(b % 2, recv_offset + b * buffer_count, real_buffer_count);

	return MPI_SUCCESS;
}
