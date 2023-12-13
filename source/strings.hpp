#pragma once
#include "defines.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#define to_lower_ascii(LETTER) ((LETTER >= 'A' && LETTER <= 'Z') ? (LETTER + 32) : LETTER)
#define u16tou8(SRC, SRCLEN, DST, DSTLEN) WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, (wchar_t*)(SRC), int(SRCLEN), (char*)(DST), int(DSTLEN), NULL, NULL)
#define u8tou16(SRC, SRCLEN, DST, DSTLEN) MultiByteToWideChar(CP_UTF8, NULL, (char*)(SRC), int(SRCLEN), (wchar_t*)(DST), int(DSTLEN))

namespace ncore {
    namespace strings {
        using string_t = std::string;
        using wstring_t = std::wstring;

        static constexpr const char const __defaultRandomCharList[] = {
                "1234567890"
                "QWERTYUIOPASDFGHJKLZXCVBNM"
                "qwertyuiopasdfghjklzxcvbnm"
        };

        class compatible_string {
        private:
            string_t _u8;
            wstring_t _u16;

        public:
            static __forceinline string_t make_u8(const wstring_t& u16) noexcept {
                auto length = u16.length();
                auto buffer = new char[length + 1] { null };

                u16tou8(u16.c_str(), length, buffer, length);

                auto result = string_t(buffer, length);
                delete[] buffer;

                return result;
            }

            static __forceinline wstring_t make_u16(const string_t& u8) noexcept {
                auto length = u8.length();
                auto buffer = new wchar_t[length + 1] { null };

                u8tou16(u8.c_str(), length, buffer, length);

                auto result = wstring_t(buffer, length);
                delete[] buffer;

                return result;
            }

        public:
            __forceinline compatible_string() = default;

            __forceinline compatible_string(const string_t& u8) noexcept {
                _u8 = u8;
                _u16 = make_u16(u8);
            }

            __forceinline compatible_string(const char* u8) noexcept {
                _u8 = u8;
                _u16 = make_u16(u8);
            }

            __forceinline compatible_string(const wstring_t& u16) noexcept {
                _u8 = make_u8(u16);
                _u16 = u16;
            }

            __forceinline compatible_string(const wchar_t* u16) noexcept {
                _u8 = make_u8(u16);
                _u16 = u16;
            }

            __forceinline compatible_string(const string_t& u8, const wstring_t& u16) noexcept {
                _u8 = u8;
                _u16 = u16;
            }

            __forceinline constexpr auto& string() const noexcept {
                return _u8;
            }

            __forceinline constexpr auto& wstring() const noexcept {
                return _u16;
            }
        };

        static __forceinline void replace_strings(string_t* data, const string_t& target, const string_t& value) {
            if (!data || target.empty() || value.empty()) return;

            do {
                auto pos = data->find(target);
                if (pos == string_t::npos) break;

                data->replace(pos, target.length(), value);
            } while (true);
        }

        static __forceinline string_t replace_strings(const string_t& data, const string_t& target, const string_t& value) noexcept {
            auto buffer = data;
            replace_strings(&buffer, target, value);
            return buffer;
        }

        static __forceinline string_t get_random_string(ssize_t length_min, ssize_t length_max = 0, const char* char_list = __defaultRandomCharList, size_t char_list_size = sizeof(__defaultRandomCharList)) {
            srand(unsigned(::time(0)) * GetCurrentThreadId());

            auto result = string_t();

            for (auto length = length_max ? ((i64_t(rand()) % i64_t(length_max - length_min - 1)) + length_min) : length_min; length > 0; length--) {
                result += char_list[rand() % (char_list_size - 1)];
            }

            return result;
        }

        static __forceinline std::vector<string_t> split_string(const string_t& data, char separator) {
            std::vector<string_t> result;
            std::stringstream stream(data.c_str());
            string_t item;

            while (std::getline(stream, item, separator)) {
                result.push_back(item);
            }

            return result;
        }

        static __forceinline constexpr string_t string_to_lower(const string_t& data) noexcept {
            auto result = string_t(data);

            for (auto& letter : result) {
                letter = std::tolower(letter);
            }

            return result;
        }

        static __forceinline const string_t& make_string_lower(string_t& data) noexcept {
            if (data.empty()) _Exit: return data;

            auto letter = byte_p(data.data());
            do *letter = std::tolower(*letter); while (*(letter++));

            goto _Exit;
        }

        template<size_t _bufferSize = 0x200> static __forceinline string_t format_string(const char* const data, ...) noexcept {
            va_list list;
            va_start(list, data);

            char buffer[_bufferSize]{ 0 };
            vsnprintf(buffer, sizeof(buffer), data, list);

            va_end(list);

            return buffer;
        }

        static __forceinline auto format_string(const char* const data, ...) noexcept {
            va_list list;
            va_start(list, data);

            auto result = string_t();

            auto length = vsnprintf(nullptr, null, data, list);
            if (length) {
                auto buffer = (char*)malloc(length += 1);

                vsnprintf(buffer, length, data, list);
                result = string_t(buffer, length - 1);

                free(buffer);
            }

            va_end(list);

            return result;
        }

        static __forceinline bool copy_string_to_clipboard(const string_t& data) {
            OpenClipboard(null);
            EmptyClipboard();

            auto length = data.length() * sizeof(wchar_t) + 1;
            auto allocation = GlobalAlloc(GMEM_MOVEABLE, length);
            if (!allocation) {
                CloseClipboard();
                return false;
            }

            u8tou16(data.c_str(), data.size(), (wchar_t*)GlobalLock(allocation), length - 1);

            GlobalUnlock(allocation);
            SetClipboardData(CF_UNICODETEXT, allocation);
            CloseClipboard();
            GlobalFree(allocation);

            return true;
        }
    }

    using namespace strings;
}