#include <cassert>
#include <thread>
#include <shmpi/shmpi.hpp>

int shmpi::shmpi_allreduce(
		shmpi::buffer* const send_buffer,
		const std::size_t offset_send,
		shmpi::buffer* const recv_buffer,
		const std::size_t offset_recv,
		const std::size_t count,
		const MPI_Datatype data_type,
		const MPI_Op op,
		MPI_Comm mpi_comm
		) {
	const auto buffer_count = (send_buffer == shmpi::shmpi_in_place) ? recv_buffer->get_buffer_count() : std::min(send_buffer->get_buffer_count(), recv_buffer->get_buffer_count());

	if (buffer_count >= count) {
		auto mpi_send_ptr = (send_buffer == shmpi::shmpi_in_place) ? MPI_IN_PLACE : send_buffer->get_ptr(0);
		auto send_shmpi_ptr = (send_buffer == shmpi::shmpi_in_place) ? recv_buffer : send_buffer;
		const auto offset = (send_buffer == shmpi::shmpi_in_place) ? offset_recv : offset_send;
		send_shmpi_ptr->read_from_device(0, offset, count);

		const auto stat = MPI_Allreduce(mpi_send_ptr, recv_buffer->get_ptr(0), count, data_type, op, mpi_comm);
		if (stat != MPI_SUCCESS) {
			return stat;
		}

		recv_buffer->write_to_device(0, offset_recv, count);
		return MPI_SUCCESS;
	}

	// When send_buffer == in_place, call recursively
	if (send_buffer == shmpi::shmpi_in_place) {
		return shmpi::shmpi_allreduce(recv_buffer, offset_recv, recv_buffer, offset_recv, count, data_type, op, mpi_comm);
	}

	// Read first buffer
	send_buffer->read_from_device(0, offset_send, buffer_count);
	std::size_t b = 0;
	for (; b < count / buffer_count - 1; b++) {
		// Read next buffer
		const auto read_next_buffer_func = [=]() {
			send_buffer->read_from_device(1 - (b % 2), offset_send + (b + 1) * buffer_count, buffer_count);
		};
		std::thread read_next_buffer_thread(read_next_buffer_func);

		// Allreduce and write
		auto send_ptr = send_buffer->get_ptr(b % 2);
		auto recv_ptr = recv_buffer->get_ptr(b % 2);
		int stat;
		if (send_ptr == recv_ptr) {
			stat = MPI_Allreduce(MPI_IN_PLACE, recv_ptr, buffer_count, data_type, op, mpi_comm);
		} else {
			stat = MPI_Allreduce(send_ptr, recv_ptr, buffer_count, data_type, op, mpi_comm);
		}
		if (stat != MPI_SUCCESS) {
			return stat;
		}
		recv_buffer->write_to_device(b % 2, offset_recv + b * buffer_count, buffer_count);

		// Join
		read_next_buffer_thread.join();
	}

	// Allreduce and write
	auto send_ptr = send_buffer->get_ptr(b % 2);
	auto recv_ptr = recv_buffer->get_ptr(b % 2);
	int stat;
	if (send_ptr == recv_ptr) {
		stat = MPI_Allreduce(MPI_IN_PLACE, recv_ptr, buffer_count, data_type, op, mpi_comm);
	} else {
		stat = MPI_Allreduce(send_ptr, recv_ptr, buffer_count, data_type, op, mpi_comm);
	}
	if (stat != MPI_SUCCESS) {
		return stat;
	}
	recv_buffer->write_to_device(b % 2, offset_recv + b * buffer_count, buffer_count);

	return stat;
}
