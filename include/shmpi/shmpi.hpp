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
	virtual void* get_ptr() const = 0;

	// Get data from device
	virtual void read_from_device(const std::size_t offset, const std::size_t count) = 0;

	// Send data to device
	virtual void write_to_device(const std::size_t offset, const std::size_t count) = 0;

	std::size_t get_buffer_count() const {return buffer_count;}
};

int shmpi_send(
		shmpi::buffer* const buffer,
		const std::size_t offset,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int dest,
		const int tag,
		MPI_Comm comm,
		const std::size_t buffer_count_arg = 0u
		);

int shmpi_recv(
		shmpi::buffer* const buffer,
		const std::size_t offset,
		const std::size_t count,
		const MPI_Datatype data_type,
		const int dest,
		const int tag,
		MPI_Comm comm,
		MPI_Status* mpi_status,
		const std::size_t buffer_count_arg = 0u
		);
} // namespace shmpi
#endif