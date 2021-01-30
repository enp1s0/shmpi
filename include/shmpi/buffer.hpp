#ifndef __SHMPI_BUFFER_HPP__
#define __SHMPI_BUFFER_HPP__
#include <cstdint>

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
	virtual void read_from_device(const unsigned buffer_id, const std::size_t mem_offset, const std::size_t count, const std::size_t buffer_offset = 0lu) = 0;

	// Send data to device
	virtual void write_to_device(const unsigned buffer_id, const std::size_t mem_offset, const std::size_t count, const std::size_t buffer_offset = 0lu) = 0;

	std::size_t get_buffer_count() const {return buffer_count;}
};
} // namespace shmpi
#endif
