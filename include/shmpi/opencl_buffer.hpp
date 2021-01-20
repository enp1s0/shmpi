#ifndef __MTK_SHMPI_OPENCL_BUFFER__
#define __MTK_SHMPI_OPENCL_BUFFER__
#include <memory>
#include <CL/cl.hpp>
#include "shmpi.hpp"

namespace shmpi {
template <class T>
class opencl_buffer : public shmpi::buffer {
	std::unique_ptr<T[]> data[2];
	cl::Buffer* cl_buffer;
	cl::CommandQueue& cl_queue;
public:
	opencl_buffer(const std::size_t buffer_count, cl::CommandQueue& cl_queue) : shmpi::buffer(buffer_count), cl_buffer(nullptr), cl_queue(cl_queue) {}
	int allocate() {
		try {
			data[0] = std::unique_ptr<T[]>(new T[shmpi::buffer::buffer_count]);
			data[1] = std::unique_ptr<T[]>(new T[shmpi::buffer::buffer_count]);
		} catch(const std::exception& e) {
			return 1;
		}
		return 0;
	}

	void* get_ptr(const unsigned buffer_id) const {
		return data[buffer_id].get();
	}

	void read_from_device(const unsigned buffer_id, const std::size_t offset, const std::size_t count) {
		cl_queue.enqueueReadBuffer(*cl_buffer, true, sizeof(T) * offset, sizeof(T) * count, data[buffer_id].get());
		cl_queue.finish();
	}

	void write_to_device(const unsigned buffer_id, const std::size_t offset, const std::size_t count) {
		cl_queue.enqueueWriteBuffer(*cl_buffer, true, sizeof(T) * offset, sizeof(T) * count, data[buffer_id].get());
		cl_queue.finish();
	}

	void set_cl_buffer(cl::Buffer* const b) {
		cl_buffer = b;
	}
};
} // namespace shmpi
#endif
