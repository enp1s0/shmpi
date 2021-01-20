#ifndef __MTK_SHMPI_TEST_CPU_BUFFER__
#define __MTK_SHMPI_TEST_CPU_BUFFER__
#include <memory>
#include <shmpi/shmpi.hpp>

namespace mtk {
namespace test {
template <class T>
class cpu_buffer : public shmpi::buffer {
	std::unique_ptr<T[]> data[2];
	T* org_ptr;
public:
	cpu_buffer(const std::size_t buffer_count) : shmpi::buffer(buffer_count) {}
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
		for (std::size_t i = 0; i < count; i++) {
			data[buffer_id].get()[i] = org_ptr[offset + i];
		}
	}

	void write_to_device(const unsigned buffer_id, const std::size_t offset, const std::size_t count) {
		for (std::size_t i = 0; i < count; i++) {
			org_ptr[offset + i] = data[buffer_id].get()[i];
		}
	}

	void set_org_ptr(T* const p) {
		org_ptr = p;
	}
};
} // namespace test
} // namespace mtk
#endif
