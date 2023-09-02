#pragma once
#include "limit.hpp"
#include "vector.hpp"
#include "readable_byte.hpp"
#include <string>
#include <windows.h>
#include <ntstatus.h>
#include "includes/ntos.h"


#define UI64_ROUND_UP(VAL, STEP)	(((DWORD64(VAL)) + STEP - 1) & (~(STEP - 1)))
#define UI64_ROUND_DOWN(VAL, STEP)	((DWORD64(VAL)) & (~(STEP - 1)))

#define PAGE_ROUND_UP(x)			UI64_ROUND_UP(x, PAGE_SIZE)
#define PAGE_ROUND_DOWN(x)			UI64_ROUND_DOWN(x, PAGE_SIZE)

namespace ncore {
	static constexpr const char const __asciiLowerUpperOffset = 'a' - 'A';


	enum class known_state_t : byte_t {
		unknown,	// ??
		known,		// 11
		halfknown	// 1?
	};

	static __forceinline constexpr known_state_t __fastcall get_known_state(readable_byte_t readable_byte) noexcept {
		if (readable_byte.dec == '??') return known_state_t::unknown;

		if (readable_byte.str[0] == '?' || readable_byte.str[1] == '?') return known_state_t::halfknown;

		return known_state_t::known;
	}

	class byte_pattern_t {
	public:
		known_state_t known_state;
		readable_byte_t readable;
		byte_t min, max;

		__forceinline constexpr byte_pattern_t(readable_byte_t readable, byte_t min, byte_t max) noexcept {
			this->known_state = get_known_state(readable);
			this->readable = readable;
			this->min = min;
			this->max = max;
		}

		__forceinline constexpr byte_pattern_t(readable_byte_t readable) noexcept {
			this->known_state = get_known_state(readable);
			this->readable = readable;
			this->min = this->max = readable.byte();
		}

		__forceinline constexpr bool __fastcall operator==(byte_t byte) const noexcept {
			switch (known_state) {
			case known_state_t::unknown: default: break;
			case known_state_t::known: return byte > min && byte < max;
			case known_state_t::halfknown: return readable == byte;
			}
			return true;
		}
	};

	class signature : public vector<short> {
	public:
		template <class _specificInfo> class multi_thread_info {
		public:
			class thread_info : public _specificInfo {
			public:
				uint32_t id;
				handle_t handle;
			};

			bool free_after_using;
			size_t threads_count = NULL;
			size_t alive_threads_count = NULL;
			thread_info* threads = NULL;

			__forceinline multi_thread_info(bool free_after_using) {
				this->free_after_using = free_after_using;
			}

			__forceinline void alloc(size_t threads_count)
			{
				auto previousInfo = threads;
				threads = new thread_info[this->threads_count = threads_count];

				if (previousInfo)
					delete[] previousInfo;
			}

			__forceinline void free() {
				if (threads)
					delete[] threads;

				threads = nullptr;
			}

			__forceinline void abort() {
				if (!threads) return;

				for (size_t i = 0; i < threads_count; i++) {
					NtTerminateThread(threads[i].handle, STATUS_SUCCESS);
					NtClose(threads[i].handle);
				}
			}
		};

		class search_thread_info {
		public:
			address_t progress;
		};

		using search_info = multi_thread_info<search_thread_info>;

	private:
		struct search_arguments
		{
			signature* instance;

			search_info::thread_info* info;

			size_t max_results_count;
			vector<address_t>* results;

			handle_t process;
			address_t start;
			size_t size;
			size_t block_size;
		};

		__forceinline constexpr void to_lower(char* str, size_t length) const noexcept {
			auto end = str + length;
			for (; str < end; str++) {
				*str = (*str >= 'A' && *str <= 'Z') ? (*str + __asciiLowerUpperOffset) : *str;
			}
		}

		__forceinline constexpr bool compare(short first, short second) const noexcept {
			char* data[2] = { (char*)&first, (char*)&second };

			if (data[0][0] != '?') {
				if (data[0][0] - data[1][0]) return false;
			}

			if (data[0][1] != '?') {
				return (data[0][1] == data[1][1]);
			}

			return true;
		}

		__forceinline constexpr short part_from_byte(byte_t b) const noexcept {
			char buffer[2] = { 0 };
			buffer[0] = __hexCharList[(b >> 4) & 0x0F];
			buffer[1] = __hexCharList[b & 0x0F];

			return *((short*)buffer);
		}

		static __declspec(noinline) void multi_thread_search(search_arguments* args) {
			auto instance = args->instance;
			auto start = (byte_t*)args->start;
			auto size = args->size;
			auto block_size = args->block_size;
			auto results = args->results;
			auto process = args->process;
			auto max_results = args->max_results_count;
			auto info = args->info;

			auto block_info = MEMORY_BASIC_INFORMATION{ 0 };
			auto block_buffer = malloc(args->block_size);

			auto block_results = vector<address_t>();
			auto block_address = address_t(0);
			auto offset = size_t(0);
			auto bytes_readed = size_t(0);
			auto reading_size = size_t(0);

		_Begin:
			if (!(offset < size && results->size() < max_results)) goto _End;

			block_info = MEMORY_BASIC_INFORMATION{ 0 };
			info->progress = block_address = address_t(start + offset);
			bytes_readed = size_t(0);

			if (NtQueryVirtualMemory(process, block_address, MemoryBasicInformation, &block_info, sizeof(block_info), NULL)) {
				goto _End;
			}

			if (block_info.State == MEM_FREE || block_info.Protect == PAGE_NOACCESS /*||
				!(block_info.Type == MEM_PRIVATE || block_info.Type == MEM_IMAGE)*/) {
			_InvalidRegion:
				if (!block_info.RegionSize) goto _End;

				bytes_readed = block_info.RegionSize;

			_Continue:
				offset += bytes_readed;

				goto _Begin;
			}

			bytes_readed = 0;
			reading_size = block_info.RegionSize - ((uint64_t)block_address - (uint64_t)block_info.BaseAddress);
			if (NtReadVirtualMemory(process, block_address, block_buffer, min(block_size, reading_size), &bytes_readed) || !bytes_readed) {
				goto _InvalidRegion;
			}

			if (instance->search(block_buffer, bytes_readed, &block_results)) {
				for (auto& result : block_results) {
					if (results->size() >= max_results) goto _End;

					results->push_back(address_t((uint64_t)block_address + ((uint64_t)result - (uint64_t)block_buffer)));
				}
			}

			goto _Continue;
		_End:

			auto handle = info->handle;
			info->handle = nullptr;
			NtClose(handle);

			free(block_buffer);

			return delete args;
		}

	public:
		__forceinline signature(const char* str = nullptr) {
			if (!str) return;

			string buffer;
			if (*str != ' ') {
				buffer = string(" ") + str;
				str = buffer.c_str();
			}

			for (; *str; str++) {
				if (*str == ' ' && *(str + 1) != '\0') {
					auto part = *((short*)(str + 1));
					to_lower((char*)&part, sizeof(short));
					push_back(part);
				}
			}
		}

		__forceinline signature(byte_t* pattern, size_t count) {
			if (!(pattern && count)) return;

			for (size_t i = 0; i < count; pattern++, i++) {
				push_back(part_from_byte(*pattern));
			}
		}

		//in data
		__forceinline size_t search(address_t start, size_t length, vector<address_t>* _results = nullptr, size_t max_results_count = size_t(-1)) {
			if (empty()) return 0;
			
			if (_results) {
				_results->clear();
			}

			auto count = size_t(0);

			auto current = (byte_t*)start - 1;
			auto end = current + length;

			while(current++ < end) {
				__try {
					if (*this != current) continue;

					if (count++ >= max_results_count) break;

					if (_results) {
						_results->push_back(current);
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					current += 0x1000;
					continue;
				}
			}

			return count;
		}

		//in external process
		__forceinline size_t search(handle_t process, address_t start, size_t length, vector<address_t>* _results = nullptr, size_t max_results_count = size_t(-1), search_info* _info = nullptr, size_t threads_count = 1, int threadsPriority = THREAD_PRIORITY_HIGHEST, size_t block_size = 0x80000) {
			if (empty()) return 0;

			if (!_results) {
				vector<address_t> buffer;
				_results = &buffer;
			}
			else {
				_results->clear();
			}

			if (!_info) {
				search_info buffer(true);
				_info = &buffer;
			}

			_info->alloc(threads_count);

			auto size_per_thread = PAGE_ROUND_UP(length / threads_count);

			for (size_t i = 0; i < threads_count; i++) {
				auto arguments = new search_arguments {
					this,

					&(_info->threads[i]),

					max_results_count,
					_results,

					process,
					address_t((uint64_t)start + (size_per_thread * i)),
					size_per_thread,
					block_size
				};

				SetThreadPriority(
					_info->threads[i].handle = CreateRemoteThread(HANDLE(-1), 0, 0,
					LPTHREAD_START_ROUTINE(multi_thread_search), LPVOID(arguments),
					0, LPDWORD(&_info->threads[i].id)), 
					threadsPriority);
			}

		_Begin:
			size_t alive_threads_count = 0;
			for (size_t i = 0; i < threads_count; i++) {
				alive_threads_count += _info->threads[i].handle != 0;
			}

			SleepEx(50, FALSE);

			if (_info->alive_threads_count = alive_threads_count) goto _Begin;

			while (_results->size() > max_results_count) {
				_results->pop_back();
			}

			if (_info->free_after_using) {
				_info->free();
			}

			return _results->size();
		}

		__forceinline bool operator==(byte_t* memory) const noexcept {
			auto t_data = begin();
			auto t_size = size();
			for (size_t i = 0; i < t_size; i++, t_data++, memory++) {
				if (!compare(*t_data, part_from_byte(*memory))) return false;
			}

			return true;
		}
	};
}
