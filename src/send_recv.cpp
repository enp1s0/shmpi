#include <shmpi/shmpi.hpp>

int shmpi::shmpi_send (
		shmpi::buffer* const buffer,
		const std::size_t offset_arg,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int dest,
		const int tag,
		MPI_Comm comm,
		const std::size_t buffer_count_arg
		) {
	const auto buffer_count = buffer_count_arg == 0 ? buffer->get_buffer_count() : buffer_count_arg;
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
		MPI_Comm comm,
		const std::size_t buffer_count_arg
		) {
	const auto buffer_count = buffer_count_arg == 0 ? buffer->get_buffer_count() : buffer_count_arg;
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
