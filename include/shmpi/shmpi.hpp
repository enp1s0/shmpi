#ifndef __SHMPI_SHMPI_HPP__
#define __SHMPI_SHMPI_HPP__
#include <cstdint>
#include <mpi.h>
#include "buffer.hpp"

namespace shmpi {
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

int shmpi_sendrecv(
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
