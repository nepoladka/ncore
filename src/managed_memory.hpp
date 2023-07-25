#pragma once
#include "defines.hpp"
#include <windows.h>
#include "includes/ntos.h"

namespace ncore {
	using memory_allocator_t = get_procedure_t(address_t, , size_t);
	using memory_reallocator_t = get_procedure_t(address_t, , address_t, size_t);
	using memory_deallocator_t = get_procedure_t(void, , address_t);

	static memory_allocator_t __managedMemoryAllocator = malloc;
	static memory_reallocator_t __managedMemoryReallocator = realloc;
	static memory_deallocator_t __managedMemoryDeallocator = free;

	__declspec(align(1)) class managed_memory {
	private:
		size_t _total_size;

		__forceinline managed_memory() noexcept { return; }
		__forceinline ~managed_memory() noexcept { return; }

	public:
		template<typename _t = byte_t> static __forceinline managed_memory* take(size_t count) noexcept {
			auto size = get_total_size<_t>(count);

			auto result = (managed_memory*)(__managedMemoryAllocator(size));
			if (!result) return nullptr;

			result->clean_all();
			result->_total_size = size;
			return result;
		}

		static __forceinline managed_memory* take(managed_memory* source) noexcept {
			if (!source) return nullptr;

			auto result = (managed_memory*)(__managedMemoryAllocator(source->_total_size));
			if (!result) return nullptr;

			result->copy(source);
			result->_total_size = source->_total_size;

			return result;
		}

		template<typename _t = byte_t> static __forceinline void resize(managed_memory** memory, size_t count) {
			if (!memory) return nullptr;

			if (!*memory) return nullptr;
			
			auto size = get_total_size<_t>(count);

			auto result = (managed_memory*)(__managedMemoryReallocator(memory, size));
			if (!result) return nullptr;

			result->_total_size = size;

			return result;
		}

		__forceinline void clean_data(int value = 0) noexcept {
			memset(data(), value, local_size());
		}

		__forceinline void clean_all(int value = 0) noexcept {
			memset(this, value, total_size());
		}

		__forceinline size_t copy(managed_memory* second) noexcept {
			if (!second) _Fail: return 0;

			auto size = min(local_size(), second->local_size());
			if (!size) goto _Fail;

			memcpy(data(), second->data(), size);

			return size;
		}

		__forceinline void release(bool clean = false) {
			if (clean)
				this->clean_all();

			return __managedMemoryDeallocator(this);
		}

		static __forceinline void release(managed_memory** memory, bool clean = false) {
			if (!memory) return;

			if (!*memory) return;

			auto instance = *memory;
			*memory = nullptr;

			return instance->release(clean);
		}

		template<typename _t> static __forceinline size_t get_local_size(size_t count) noexcept {
			return sizeof(_t) * count;
		}

		template<typename _t> static __forceinline size_t get_total_size(size_t count) noexcept {
			return sizeof(size_t) + get_local_size<_t>(count);
		}

		__forceinline size_t local_size() const noexcept {
			return _total_size - sizeof(size_t);
		}

		__forceinline size_t total_size() const noexcept {
			return _total_size;
		}

		template<typename _t> __forceinline size_t count() const noexcept {
			return local_size() / sizeof(_t);
		}

		template<typename _t = byte_t> __forceinline _t* data() const noexcept {
			return ((_t*)((byte_t*)this + sizeof(size_t)));
		}

		template<typename _t = byte_t> __forceinline _t& at(size_t index) {
			return *(data<_t>() + index);
		}
	};

	template<typename _t = byte_t> struct managed_array {
	protected:
		managed_memory* _memory = nullptr;
		bool _release_on_destroy = false;

	public:
		__forceinline managed_array() = default;

		__forceinline managed_array(size_t capacity, bool release_on_destroy = true) noexcept {
			_memory = managed_memory::take<_t>(capacity);
			_release_on_destroy = release_on_destroy;
		}

		__forceinline ~managed_array() noexcept {
			release();
		}

		__forceinline void release(bool clean = true) {
			if (_memory && _release_on_destroy) return managed_memory::release(&_memory, clean);
		}

		__forceinline size_t capacity() const noexcept {
			if (!_memory) return 0;

			return _memory->count<_t>();
		}

		__forceinline _t* data() const noexcept {
			if (!_memory) return nullptr;

			return _memory->data<_t>();
		}

		__forceinline _t& operator[](size_t index) const noexcept {
			return _memory->at<_t>(index);
		}

		__forceinline managed_array& operator=(managed_array& source) noexcept {
			_release_on_destroy = source._release_on_destroy;
			source._release_on_destroy = false;

			_memory = source._memory;

			return *this;
		}

		__forceinline managed_array& operator=(const managed_array& source) noexcept {
			_release_on_destroy = false;
			_memory = source._memory;
			return *this;
		}
	};
}
