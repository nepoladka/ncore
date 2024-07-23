#pragma once
#include <windows.h>
#include <string>

#include "defines.hpp"

namespace ncore {
	class shared_memory {
	private:
		struct {
			byte_t* address = nullptr;
			size_t size = 0;

			handle_t handle = nullptr;
			bool allocated = false;

			std::string name = std::string();

			__forceinline bool allocate(const std::string& name, size_t size) {
				if (!(handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, NULL, DWORD(size), (this->name = name).c_str()))) return false;

				if (address = (byte_t*)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, NULL, NULL, size)) return true;

				CloseHandle(handle);

				return false;
			}

			__forceinline bool attach(const std::string& name, size_t size) {
				if (!(handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, (this->name = name).c_str()))) return false;

				if (address = (byte_t*)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, NULL, NULL, size)) return true;

				CloseHandle(handle);

				return false;
			}

			__forceinline void detach() {
				if (!handle) return;

				CloseHandle(handle);
				handle = nullptr;
			}

			__forceinline bool deallocate() {
				if (!allocated) return true;

				if (address) {
					UnmapViewOfFile(address);
					address = nullptr;
				}

				return true;
			}
		}_wrapper;

		__forceinline shared_memory(const std::string& memoryName, size_t memorySize, bool allocate) {
			if (allocate)
				_wrapper.allocated = _wrapper.allocate(memoryName, memorySize);
			else
				_wrapper.attach(memoryName, memorySize);
		}

		__forceinline ~shared_memory() {
			if (_wrapper.allocated)
				_wrapper.allocated = !_wrapper.deallocate();

			_wrapper.detach();
		}

	public:
		static __forceinline shared_memory* create(const std::string& memoryName, size_t memorySize) {
			auto res = new shared_memory(memoryName, memorySize, true);
			if (res->valid()) return res;
			delete res;
			return nullptr;
		}

		static __forceinline shared_memory* attach(const std::string& memoryName, size_t memorySize) {
			auto res = new shared_memory(memoryName, memorySize, false);
			if (res->valid()) return res;
			delete res;
			return nullptr;
		}

		static __forceinline void terminate(shared_memory** overlay) {
			if (!overlay) return;

			auto instance = *overlay;
			*overlay = nullptr;

			if (instance) return delete instance;
		}

		template<typename _t = byte_t> __forceinline _t* data() {
			return (_t*)_wrapper.address;
		}

		__forceinline size_t size() {
			return _wrapper.allocated ? _wrapper.size : 0;
		}

		__forceinline bool valid() {
			__try {
				if (!(_wrapper.address && _wrapper.handle)) return false;

				auto first_byte = *data() + 1;
				return true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return false;
			}
		}

		__forceinline handle_t handle() {
			return _wrapper.handle;
		}
	};
}
