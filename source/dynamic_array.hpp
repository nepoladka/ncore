#pragma once
#include "managed_memory.hpp"

namespace ncore {
	template<typename _t> class dynamic_array : public managed_array<_t> {
	public:
		size_t count = 0;

		__forceinline dynamic_array() : managed_array<_t>() { return; }

		__forceinline dynamic_array(size_t capacity) : managed_array<_t>(capacity) { return; }

		__forceinline void resize(size_t capacity) {
			return managed_memory::resize<_t>(&managed_array<_t>::_memory, capacity);
		}

		__forceinline dynamic_array& operator=(dynamic_array& source) {
			managed_array<_t>::operator=(source);
			count = source.count;
			return *this;
		}

		__forceinline dynamic_array& operator=(const dynamic_array& source) {
			managed_array<_t>::operator=(source);
			count = source.count;
			return *this;
		}
	};
}
