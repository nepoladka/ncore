#pragma once
#include "defines.hpp"
#include "environment.hpp"

#define module_index_ntdll 1
#define module_index_kernel32 2
#define module_index_kernelbase 3

#define hidden_import(MODULE_INDEX, PROCEDURE_ORDINAL, PROCEDURE) ((decltype(PROCEDURE)*)(ncore::hidden::import(MODULE_INDEX, PROCEDURE_ORDINAL).address))

namespace ncore::hidden {
	class string {
	private:
		const char* _source;

	public:
		__forceinline string(const char* ptr) {
			_source = ptr;
		}

		__forceinline constexpr const char* c_str() const noexcept {
			return _source;
		}

		__forceinline constexpr size_t size() const noexcept {
			if (_source) return strlen(_source);

			return null;
		}

		__forceinline constexpr bool valid() const noexcept {
			return size();
		}

		__forceinline void release() noexcept {
			auto address = address_t(_source);
			auto size = this->size();
			auto capacity = size;
			auto current_protection = null;

			if (NT_SUCCESS(NtProtectVirtualMemory(__current_process, &address, &capacity, PAGE_EXECUTE_READWRITE, PULONG(&current_protection)))) {
				memset(address_t(_source), null, size);
				_source = nullptr;

				NtProtectVirtualMemory(__current_process, &address, &capacity, current_protection, nullptr);
			}
		}
	};

	class import {
	public:
		address_t address = nullptr;

		__forceinline import(ui16_t module_index, ui16_t procedure_ordinal) noexcept {
			auto ldr = __process_environment->Ldr;

			auto index = ui16_t(0);

			auto head = ldr->InLoadOrderModuleList.Flink;
			auto current = head;
			do {
				auto entry = *PLDR_DATA_TABLE_ENTRY(current);
				current = entry.InLoadOrderLinks.Flink;

				if (index++ != module_index || !entry.DllBase) continue;

				auto image_base = ui64_t(entry.DllBase);
				auto image = *PIMAGE_DOS_HEADER(image_base);
				if (image.e_magic != IMAGE_DOS_SIGNATURE) _Fail: break;

				auto headers = *PIMAGE_NT_HEADERS(image_base + image.e_lfanew);
				if (headers.Signature != IMAGE_NT_SIGNATURE) goto _Fail;

				auto export_directory_offset = headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
				if (!export_directory_offset) goto _Fail;

				auto export_directory = *PIMAGE_EXPORT_DIRECTORY(image_base + export_directory_offset);

				auto addresses_table = (ui32_t*)(image_base + export_directory.AddressOfFunctions);
				auto names_table = (ui32_t*)(image_base + export_directory.AddressOfNames);
				auto ordinals_table = (ui16_t*)(image_base + export_directory.AddressOfNameOrdinals);

				for (size_t i = 0; i < export_directory.NumberOfNames; i++) {
					auto ordinal = *ui16_p(ordinals_table + i);
					if (ordinal != procedure_ordinal) continue;

					address = address_t(image_base + *ui32_p(addresses_table + ordinal));

					return;
				}

				goto _Fail;
			} while (head != current);
		}
	};
}