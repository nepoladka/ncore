#pragma once
#include "defines.hpp"

#ifndef NCORE_ENUMERATION_NO_BINARY
#include "environment.hpp"
#endif

namespace ncore::enumeration {
	enum class return_t : bool { next = false, skip = false, stop = true };

	template<typename item_t, typename data_t = void*> using procedure_t = get_procedure_t(return_t, , size_t& index, item_t& item, data_t data);
	template<typename item_t, typename data_t = void*, typename result_t = bool> using procedure_ex_t = get_procedure_t(return_t, , size_t& index, item_t& item, data_t data, result_t& result);
	template<typename data_t = void*> using binary_procedure_t = get_procedure_t(return_t, , address_t address, data_t data, size_t& step);

	template<typename item_t, typename data_t = void*> ncore_procedure(bool) enumerate(item_t* elements, size_t count, procedure_t<item_t, data_t> procedure, data_t data, size_t step = 1, size_t failure_step = 0) noexcept {
		if (!step) {
			step = 1;
		}

		if (count && procedure) for (size_t i = 0; i < count; i += step, elements += step) {
			__try {
				if (bool(procedure(i, *elements, data))) return true;
			}
			__except (1) {
				i += failure_step;
				elements += failure_step;

				continue;
			}
		}
		return false;
	}

	template<typename item_t, typename data_t = void*, typename result_t = bool> ncore_procedure(result_t) enumerate_ex(item_t* elements, size_t count, procedure_ex_t<item_t, data_t, result_t> procedure, data_t data, size_t step = 1, size_t failure_step = 0) noexcept {
		if (!step) {
			step = 1;
		}

		auto result = result_t();

		if (count && procedure) for (size_t i = 0; i < count; i += step, elements += step) {
			__try {
				if (bool(procedure(i, *elements, data, &result))) return result;
			}
			__except (true) {
				i += failure_step;
				elements += failure_step;

				continue;
			}
		}

		return result;
	}

#ifndef NCORE_ENUMERATION_NO_BINARY
	template<typename data_t = void*> ncore_procedure(constexpr bool) enumerate_binary(address_t address, size_t size, binary_procedure_t<data_t> procedure, data_t data) noexcept {
		struct enumeration_info {
			data_t* data;
			binary_procedure_t<data_t> procedure;
		};
		
		auto enumeration_procedure = [](size_t& i, byte_t& byte, enumeration_info* info) noexcept {
			__try {
				auto step = size_t(0);
				auto result = info->procedure(&byte, *info->data, step);

				if (step) {
					i += step;
				}

				return result;
			}
			__except (true) {
				auto region_info = MEMORY_BASIC_INFORMATION();
				NtQueryVirtualMemory(CURRENT_PROCESS_HANDLE, &byte, MEMORY_INFORMATION_CLASS::MemoryBasicInformation, &region_info, sizeof(region_info), nullptr);

				i += region_info.RegionSize ? region_info.RegionSize : PAGE_SIZE;
			}

			return return_t::next;
		};

		auto info = enumeration_info{ &data, procedure };

		return enumerate<byte_t, enumeration_info*>((byte_t*)address, size, enumeration_procedure, &info);
	}
#endif
}
