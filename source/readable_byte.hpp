#pragma once
#include "defines.hpp"
#include "strings.hpp"

namespace ncore {
	static constexpr const unsigned char const __hexCharList[] = { "0123456789abcdef" };

	union readable_byte_t {
		char str[2];
		short dec;

		__forceinline constexpr __fastcall readable_byte_t(short dec = '00') noexcept {
			this->dec = dec;
		}

		__forceinline constexpr __fastcall readable_byte_t(byte_t byte) noexcept {
			str[0] = __hexCharList[(byte >> 4) & 0x0F];
			str[1] = __hexCharList[byte & 0x0F];
		}

		__forceinline constexpr byte_t __fastcall byte() const noexcept {
			return (str[0] % 32 + 9) % 25 * 16 + (str[1] % 32 + 9) % 25;
		}

		__forceinline constexpr bool __fastcall operator==(byte_t byte) const noexcept {
			if (dec == '??') return true;

			auto second = readable_byte_t(byte);

			if (str[0] != '?') {
				if (str[0] != second.str[0]) return false;
			}

			if (str[1] != '?') {
				if (str[1] != second.str[1]) return false;
			}

			return true;
		}
	};

	class readable_byte_array {
	private:
		std::string _readable;
		char _separator;
		size_t _bytes_count;

		std::vector<readable_byte_t> _data;

	public:
		__forceinline readable_byte_array(const std::string& readable, char separator = ' ') noexcept {
			auto parts = strings::split_string(readable, separator);

			for (auto& part : parts) {
				_data.push_back({ *(short*)part.data() });
			}

			_readable = readable;
			_separator = separator;
			_bytes_count = _data.size();
		}

		__forceinline readable_byte_array(const void* bytes, size_t length, char separator = ' ') noexcept {
			auto readable = std::string();

			for (auto current = byte_p(bytes), end = byte_p(bytes) + length; current != end; current++) {
				auto byte = readable_byte_t(*current);
				
				readable += { byte.str[0], byte.str[1], separator };
				_data.push_back(byte);
			}

			_readable = readable.substr(0, readable.length() - 1);
			_separator = separator;
			_bytes_count = _data.size();
		}

		__forceinline std::vector<byte_t> bytes() noexcept {
			auto result = std::vector<byte_t>();
			for (auto& part : _data) {
				result.push_back(part.byte());
			}
			return result;
		}

		__forceinline std::string readable() noexcept {
			return _readable;
		}
	};
}
