#pragma once
#include "vector.hpp"

#ifndef BEA_ENGINE_STATIC
#define BEA_ENGINE_STATIC
#endif

#ifndef BEA_USE_STDCALL
#define BEA_USE_STDCALL
#endif

#include "includes/beaengine/beaengine.h"
#pragma comment(lib, "beaengine.lib")

namespace ncore {
	using namespace std;

	using disasm_t = DISASM;

	struct disassembled {
		int length;
		disasm_t info;

		__forceinline bool equals(const disassembled& second) const noexcept {
			return !strcmp(info.Instruction.Mnemonic, second.info.Instruction.Mnemonic);
		}

		__forceinline bool operator==(const disassembled& second) const noexcept {
			return equals(second);
		}
	};

	class disassembled_code : public std::vector<disassembled> {
	public:
		__forceinline disassembled_code(byte_t* code, size_t length) : std::vector<disassembled>() {
			auto disasm = disasm_t{ 0 };
			disasm.Archi = 64;
			disasm.EIP = UIntPtr(code);

			disasm.VirtualAddr = 0;
			while (disasm.VirtualAddr < length) {
				auto length = Disasm(&disasm);
				if (length <= 0) return;

				push_back({ length, disasm });

				disasm.EIP += length;
				disasm.VirtualAddr += length;
			}
		}

		__forceinline size_t length() const noexcept {
			auto result = size_t(0);
			for (auto& part : *this) {
				result += part.length;
			}
			return result;
		}

		__forceinline bool equals(const disassembled_code& second) const noexcept {
			auto leftPart = begin();
			auto rightPart = second.begin();

			auto length = min(size(), second.size());
			if (!length) return false;

			for (size_t i = 0; i < length; i++, leftPart++, rightPart++) {
				if (!leftPart->equals(*rightPart)) return false;
			}

			return true;
		}

		__forceinline bool operator==(const disassembled_code& second) const noexcept {
			return equals(second);
		}
	};
}
