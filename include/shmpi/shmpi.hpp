#ifndef __SHMPI_SHMPI_HPP__
#define __SHMPI_SHMPI_HPP__
#include <functional>
#include <mpi.h>

namespace shmpi {
class buffer {
protected:
	// size of buffer
	const std::size_t buffer_count;
public:
	buffer(const std::size_t buffer_count) : buffer_count(buffer_count) {}
	virtual int allocate() = 0;

	// getter
	virtual void* get_ptr(const unsigned buffer_id) const = 0;

	// Get data from device
	virtual void read_from_device(const unsigned buffer_id, const std::size_t offset, const std::size_t count) = 0;

	// Send data to device
	virtual void write_to_device(const unsigned buffer_id, const std::size_t offset, const std::size_t count) = 0;

	std::size_t get_buffer_count() const {return buffer_count;}
};

static shmpi::buffer* const shmpi_in_place = reinterpret_cast<shmpi::buffer*>(1lu);

int shmpi_send(
		shmpi::buffer* const buffer,
		const std::size_t offset,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int dest,
		const int tag,
		MPI_Comm comm
		);

int shmpi_recv(
		shmpi::buffer* const buffer,
		const std::size_t offset,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int dest,
		const int tag,
		MPI_Comm comm
		);

int shmpi_allreduce(
		shmpi::buffer* const send_buffer,
		const std::size_t offset_send,
		shmpi::buffer* const recv_buffer,
		const std::size_t offset_recv,
		const std::size_t count,
		const MPI_Datatype data_type,
		const MPI_Op op,
		MPI_Comm comm
		);

int shmpi_alltoall(
		shmpi::buffer* const send_buffer,
		const std::size_t offset_send,
		const std::size_t send_count,
		MPI_Datatype send_type,
		shmpi::buffer* const recv_buffer,
		const std::size_t offset_recv,
		const std::size_t recv_count,
		MPI_Datatype recv_type,
		MPI_Comm comm
		);
} // namespace shmpi
#endif
