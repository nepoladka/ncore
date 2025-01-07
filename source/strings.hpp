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

        static constexpr const auto const __xorTime = __TIME__;
        static constexpr const auto const __xorSeed = static_cast<int>(__xorTime[7]) + static_cast<int>(__xorTime[6]) * 10 + static_cast<int>(__xorTime[4]) * 60 + static_cast<int>(__xorTime[3]) * 600 + static_cast<int>(__xorTime[1]) * 3600 + static_cast<int>(__xorTime[0]) * 36000;

        class compatible_string {
        private:
            string_t _u8;
            wstring_t _u16;

        public:
            static __forceinline string_t make_u8(const wstring_t& u16) noexcept {
                auto length = u16.length();
                auto buffer = new char[length + 2] { null };

                u16tou8(u16.c_str(), length, buffer, length + 2);

                auto result = string_t(buffer, length + 2);
                delete[] buffer;

                return result.c_str();
            }

            static __forceinline wstring_t make_u16(const string_t& u8) noexcept {
                auto length = u8.length();
                auto buffer = new wchar_t[length + 1] { null };

                u8tou16(u8.c_str(), length, buffer, length);

                auto result = wstring_t(buffer, length);
                delete[] buffer;

                return result.c_str();
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

        class xored_string {
        private:
            // 1988, Stephen Park and Keith Miller
            // "Random Number Generators: Good Ones Are Hard To Find", considered as "minimal standard"
            // Park-Miller 31 bit pseudo-random number generator, implemented with G. Carta's optimisation:
            // with 32-bit math and without division

            template < int N >
            struct random_generator {
            private:
                static constexpr unsigned a = 16807; // 7^5
                static constexpr unsigned m = 2147483647; // 2^31 - 1

                static constexpr unsigned s = random_generator< N - 1 >::value;
                static constexpr unsigned lo = a * (s & 0xFFFF); // Multiply lower 16 bits by 16807
                static constexpr unsigned hi = a * (s >> 16); // Multiply higher 16 bits by 16807
                static constexpr unsigned lo2 = lo + ((hi & 0x7FFF) << 16); // Combine lower 15 bits of hi with lo's upper bits
                static constexpr unsigned hi2 = hi >> 15; // Discard lower 15 bits of hi
                static constexpr unsigned lo3 = lo2 + hi;

            public:
                static constexpr unsigned max = m;
                static constexpr unsigned value = lo3 > m ? lo3 - m : lo3;
            };

            template <>
            struct random_generator< 0 > {
                static constexpr unsigned value = __xorSeed;
            };

            template < int N, int M >
            struct random_int {
                static constexpr auto value = random_generator< N + 1 >::value % M;
            };

            template < int N >
            struct random_char {
                static const char value = static_cast<char>(1 + random_int< N, 0x7F - 1 >::value);
            };

        public:
            template < size_t N, int K >
            struct xor_string {
            private:
                const char _key;
                std::array< char, N + 1 > _encrypted;

                constexpr char enc(char c) const {
                    return c ^ _key;
                }

                char dec(char c) const {
                    return c ^ _key;
                }

            public:
                template < size_t... Is >
                constexpr __forceinline xor_string(const char* str, std::index_sequence< Is... >) :
                    _key(random_char< K >::value), _encrypted{ enc(str[Is])... } {
                    return;
                }

                __forceinline decltype(auto) decrypt(void) {
                    for (size_t i = 0; i < N; ++i) {
                        _encrypted[i] = dec(_encrypted[i]);
                    }
                    _encrypted[N] = '\0';
                    return _encrypted.data();
                }
            };
        };

        static __forceinline auto replace_strings(string_t* data, const string_t& target, const string_t& value) noexcept {
            if (!data || target.empty()) return data;

            do {
                auto pos = data->find(target);
                if (pos == string_t::npos) break;

                data->replace(pos, target.length(), value);
            } while (true);

            return data;
        }

        static __forceinline string_t replace_strings(const string_t& data, const string_t& target, const string_t& value) noexcept {
            auto buffer = data;
            replace_strings(&buffer, target, value);
            return buffer;
        }

        static __forceinline string_t get_random_string(ssize_t length_min, ssize_t length_max = 0, const char* char_list = __defaultRandomCharList, size_t char_list_size = sizeof(__defaultRandomCharList)) noexcept {
            srand(unsigned(::time(0)) * GetCurrentThreadId());

            auto result = string_t();

            for (auto length = length_max ? ((i64_t(rand()) % i64_t(length_max - length_min - 1)) + length_min) : length_min; length > 0; length--) {
                result += char_list[rand() % (char_list_size - 1)];
            }

            return result;
        }

        static __forceinline std::vector<string_t> split_string(const string_t& string, size_t length) {
            auto results = std::vector<string_t>();

            auto position = size_t(0);
            while (position < string.length()) {
                results.push_back(string.substr(position, length));
                position += length;
            }

            return results;
        }

        static __forceinline std::vector<string_t> split_string(const string_t& string, char separator) noexcept {
            std::vector<string_t> result;
            std::stringstream stream(string.c_str());
            string_t item;

            while (std::getline(stream, item, separator)) {
                result.push_back(item);
            }

            return result;
        }

        static __forceinline std::vector<string_t> split_string(const string_t& string, const string_t& separator) noexcept {
            std::vector<string_t> results;
            size_t start = 0;
            size_t end = 0;

            while ((end = string.find(separator, start)) != string_t::npos) {
                results.push_back(string.substr(start, end - start));
                start = end + separator.length();
            }

            results.push_back(string.substr(start));

            return results;
        }
        
        //static __forceinline string_t get_inner_of_string(const string_t& text, const string_t& begin, const string_t& end) noexcept {
        //    size_t beginPos = text.find(begin);
        //    if (beginPos == std::string::npos) _Fail: return { };
        //
        //    beginPos += begin.length();
        //
        //    size_t endPos = text.find(end, beginPos);
        //    if (endPos == std::string::npos) goto _Fail;
        //
        //    return text.substr(beginPos, endPos - beginPos);
        //}

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

        template<ui32_t _first = 2166136261u, ui32_t _second = 16777619u>
        static __forceinline constexpr ui32_t hash_string(char const* s, size_t l) noexcept {
            return ((l ? hash_string(s, l - 1) : _first) ^ s[l]) * _second;
        }

        template<ui32_t _first = 2166136261u, ui32_t _second = 16777619u>
        static __forceinline constexpr ui32_t hash_string(const string_t& target) noexcept {
            return hash_string<_first, _second>(target.c_str(), target.length());
        }



    }

    using namespace strings;
}



//--------------------------------------------------------------------------------
//-- Note: XorStr will __NOT__ work directly with functions like printf.
//         To work with them you need a wrapper function that takes a const char*
//         as parameter and passes it to printf and alike.
//
//         The Microsoft Compiler/Linker is not working correctly with variadic 
//         templates!
//  
//         Use the functions below or use std::cout (and similar)!
//--------------------------------------------------------------------------------

static auto w_printf = [](const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_s(fmt, args);
    va_end(args);
    };

static auto w_printf_s = [](const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_s(fmt, args);
    va_end(args);
    };

static auto w_sprintf = [](char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buf, sizeof(buf), fmt, args);
    va_end(args);
    };

static auto w_sprintf_s = [](char* buf, size_t buf_size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buf, buf_size, fmt, args);
    va_end(args);
    };

//c++ 20 -> u8"sample u8 text" has a (const char8_t*) type, this operator makes it (const char*) - u8"sample u8 text"d (d means default)
static __forceinline constexpr const char* const operator"" d(const char8_t* c, unsigned __int64 l) noexcept {
    return (const char*)c;
}

//const hashing
static __forceinline constexpr unsigned __int32 operator"" h(char const* s, unsigned __int64 l) noexcept {
    return ncore::strings::hash_string(s, l);
}

#define __cstrh(VALUE) ncore::types::stored_const<unsigned __int32, ("" VALUE ""h)>::value //const string hash
#define __iscstrheq(HASH, VALUE) __cstrh(HASH) == VALUE //is const string hash eqauls

#ifdef NCORE_STRINGS_NO_XOR
#define __cstrx(VALUE) (VALUE)
#else
#define __cstrx(VALUE) (ncore::strings::xored_string::xor_string<sizeof(VALUE) - 1, __COUNTER__>(VALUE, std::make_index_sequence< sizeof(VALUE) - 1>()).decrypt())
#endif
