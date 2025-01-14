#pragma once
#include "defines.hpp"
#include "enumeration.hpp"

#include <vector>
#include <string>

#ifndef BEA_ENGINE_STATIC
#define BEA_ENGINE_STATIC
#endif

#ifndef BEA_USE_STDCALL
#define BEA_USE_STDCALL
#endif

#include "includes/beaengine/beaengine.h"

#ifdef NCORE_DISASSEMBLY_INCLUDE_BEA_LIBRARY
#pragma comment(lib, "beaengine.lib")
#endif

namespace ncore::disassembly {
	begin_unaligned struct instruction {
	public:
		using collection = std::vector<instruction>;

		ui64_t eip, offset;
		ui32_t security_block;
		char complete_instruction[INSTRUCT_LENGTH];
		ui32_t architecture;
		ui64_t options;
		INSTRTYPE info;
		OPTYPE operands[9];
		PREFIXINFO prefix;
		i32_t error;

	private:
		InternalDatas _internal;

	public:
		address_t address;
		i32_t length;
		byte_t* saved_bytes;

		__forceinline void __fastcall save_bytes(byte_t* bytes) noexcept {
			auto pointer = saved_bytes ? saved_bytes : nullptr;

			memcpy(saved_bytes = (byte_t*)malloc(length), bytes, length);

			if (pointer) return free(pointer);
		}

		__forceinline constexpr void __fastcall release_bytes() noexcept {
			if (!saved_bytes) return;

			auto pointer = saved_bytes;
			saved_bytes = nullptr;

			return free(pointer);
		}

		//name() == "call" !ONLY!
		__forceinline constexpr address_t __fastcall call_destination() const noexcept {
			return (length == 6) ?
				address_t(ui64_t(address) + info.AddrValue - offset) : 
				address_t(info.AddrValue - offset);
		}

		__forceinline constexpr std::string __fastcall readable() const noexcept {
			return complete_instruction;
		}

		__forceinline constexpr std::string __fastcall name() const noexcept {
			return info.Mnemonic;
		}

		__forceinline bool __fastcall equals(const instruction& second) const noexcept {
			return !strcmp(info.Mnemonic, second.info.Mnemonic);
		}

		__forceinline bool __fastcall operator==(const instruction& second) const noexcept {
			return equals(second);
		}
	} end_unaligned;

	class code : private instruction::collection {
	public:
		struct info_t {
			std::string name;
			std::vector<instruction> references;
		};

		template<typename data_t = void*> using enumeration_procedure_t = get_procedure_t(enumeration::return_t, , const index_t index, const instruction& instruction, data_t data);
		using bytes_getting_procedure_t = get_procedure_t(bool, , address_t address, byte_p* buffer, size_t size);
		using bytes_releasing_procedure_t = get_procedure_t(bool, , byte_p* buffer, size_t size);

		address_t address;

		__forceinline code(const byte_p code, size_t length, address_t address = nullptr, bool save_bytes = false) noexcept {
			this->address = address = (address ? address : address_t(code));

			auto instruction = disassembly::instruction();
			instruction.architecture = 64;
			instruction.eip = ui64_t(code);

			instruction.offset = 0;
			while (instruction.offset < length) {
				if ((instruction.length = Disasm(LPDISASM(&instruction))) <= 0) return;

				if (save_bytes)
					instruction.save_bytes(code + instruction.offset);

				instruction.address = address_t(ui64_t(address) + instruction.offset);
				instruction::collection::push_back(instruction);

				instruction.eip += instruction.length;
				instruction.offset += instruction.length;
			}
		}

		__forceinline constexpr const size_t length() const noexcept {
			auto result = size_t(0);
			for (auto& part : *this) {
				result += part.length;
			}
			return result;
		}

		__forceinline constexpr const size_t size() const noexcept {
			return instruction::collection::size();
		}

		__forceinline std::string readable(const std::string& separator = "\n") const noexcept {
			auto result = std::string();

			for (auto& instruction : *this) {
				result += separator + instruction.readable();
			}

			return result.substr(separator.length());
		}

		__forceinline constexpr auto data() noexcept {
			return instruction::collection::data();
		}

		__forceinline constexpr const auto data() const noexcept {
			return instruction::collection::data();
		}

		__forceinline constexpr auto at(size_t index) noexcept {
			return instruction::collection::at(index);
		}

		__forceinline constexpr const auto& at(size_t index) const noexcept {
			return instruction::collection::at(index);
		}

		__forceinline constexpr const auto begin() const noexcept {
			return instruction::collection::begin();
		}

		__forceinline constexpr const auto end() const noexcept {
			return instruction::collection::end();
		}


		__forceinline std::vector<info_t> summary() const noexcept {
			auto result = std::vector<info_t>();

			auto check_name = [&](const instruction& instruction) {
				for (auto& reference : result) {
					if (reference.name == instruction.name()) {
						reference.references.push_back(instruction);
						return true;
					}
				}

				result.push_back({ instruction.name(), {instruction} });

				return false;
			};

			for (auto& instruction : *this) {
				check_name(instruction);
			}

			return result;
		}

		template<typename data_t = void*> __forceinline bool enumerate(enumeration_procedure_t<data_t> procedure, data_t data) const noexcept {
			struct enumeration_info {
				enumeration_procedure_t<data_t> procedure;
				data_t* data;
			};

			auto info = enumeration_info{
				procedure,
				&data
			};

			static auto enumeration_procedure = [](size_t& index, const instruction& instruction, enumeration_info* info) noexcept {
				return info->procedure(index, instruction, *info->data);
			};

			return enumeration::enumerate<const instruction, enumeration_info*>(this->data(), this->size(), enumeration_procedure, &info);
		}

		__forceinline std::vector<instruction> search_by_name(const std::string& name) const noexcept {
			struct enumration_info {
				const std::string* name;
				std::vector<instruction>* result;
			};

			auto result = std::vector<instruction>();

			auto info = enumration_info{
				&name,
				&result
			};

			static auto enumeration_procedure = [](const size_t, const instruction& instruction, enumration_info* info) noexcept {
				if (instruction.name() == *info->name) {
					info->result->push_back(instruction);
				}
				return enumeration::return_t::next;
			};

			enumerate<enumration_info*>(enumeration_procedure, &info);

			return result;
		}

		__forceinline bool equals(const code& second) const noexcept {
			static auto enumeration_procedure = [](const size_t i, const instruction& instruction, const code* second) noexcept {
				return enumeration::return_t(!instruction.equals(second->at(i)));
			};

			return !enumerate<const code*>(enumeration_procedure, &second);
		}

		__forceinline std::vector<code> calls(bytes_getting_procedure_t getting, bytes_releasing_procedure_t releasing, size_t length = 0x100) noexcept {
			struct enumration_info {
				bytes_getting_procedure_t take_bytes;
				bytes_releasing_procedure_t release_bytes;
				size_t length;
				std::vector<code>* result;
			};

			auto result = std::vector<code>();
			if (!(getting && releasing && length)) _Exit: return result;

			auto info = enumration_info{
				getting, releasing,
				length,
				&result
			};

			static auto enumeration_procedure = [](const size_t, const instruction& instruction, enumration_info* info) noexcept {
				if (instruction.name() == "call") {
					auto buffer = byte_p();
					auto destination = address_t(instruction.call_destination());

					if (info->take_bytes(destination, &buffer, info->length)) {
						info->result->push_back(code(buffer, info->length, destination));

						info->release_bytes(&buffer, info->length);
					}
				}

				return enumeration::return_t::next;
			};

			enumerate<enumration_info*>(enumeration_procedure, &info);

			goto _Exit;
		}

		__forceinline instruction& operator[](size_t index) noexcept {
			return instruction::collection::operator[](index);
		}

		__forceinline constexpr const instruction& operator[](size_t index) const noexcept {
			return instruction::collection::operator[](index);
		}

		__forceinline bool operator==(const code& second) const noexcept {
			return equals(second);
		}
	};
}
