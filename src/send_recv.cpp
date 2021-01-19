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
		buffer->read_from_device(offset_arg, count);
		return MPI_Send(buffer->get_ptr(), count, data_type, dest, tag, comm);
	}

	for (std::size_t b = 0; b < count / buffer_count; b++) {
		const auto offset = b * buffer_count;
		const auto real_buffer_size = std::min(count - offset, buffer_count);
		buffer->read_from_device(offset + offset_arg, real_buffer_size);
		const auto stat = MPI_Send(buffer->get_ptr(), real_buffer_size, data_type, dest, tag, comm);
		if (stat != MPI_SUCCESS) {
			return stat;
		}
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
		MPI_Status* mpi_status,
		const std::size_t buffer_count_arg
		) {
	const auto buffer_count = buffer_count_arg == 0 ? buffer->get_buffer_count() : buffer_count_arg;
	if (count <= buffer_count) {
		const auto stat = MPI_Recv(buffer->get_ptr(), count, data_type, source, tag, comm, mpi_status);
		buffer->write_to_device(offset_arg, count);
		return stat;
	}

	int stat = 0;
	for (std::size_t b = 0; b < count / buffer_count; b++) {
		const auto offset = b * buffer_count;
		const auto real_buffer_size = std::min(count - offset, buffer_count);
		const auto stat = MPI_Recv(buffer->get_ptr(), real_buffer_size, data_type, source, tag, comm, mpi_status);
		if (stat != MPI_SUCCESS) {
			return stat;
		}
		buffer->write_to_device(offset + offset_arg, real_buffer_size);
	}
	return MPI_SUCCESS;
}
