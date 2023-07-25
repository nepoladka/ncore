#pragma once
#include "defines.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#define to_lower_ascii(CHAR) ((CHAR >= 'A' && CHAR <= 'Z') ? (CHAR + 32) : CHAR)
#define u16tou8(SRC, SRCLEN, DST, DSTLEN) WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, SRC, SRCLEN, DST, DSTLEN, NULL, NULL)
#define u8tou16(SRC, SRCLEN, DST, DSTLEN) MultiByteToWideChar(CP_UTF8, NULL, SRC, SRCLEN, DST, DSTLEN)

namespace ncore::string_utils {
    static constexpr const char const __defaultRandomCharList[] = {
            "1234567890"
            "QWERTYUIOPASDFGHJKLZXCVBNM"
            "qwertyuiopasdfghjklzxcvbnm"
    };

    static __forceinline void replace(std::string* data, const std::string& target, const std::string& value) {
        if (!data || target.empty() || value.empty()) return;

        do {
            auto pos = data->find(target);
            if (pos == std::string::npos) break;

            data->replace(pos, target.length(), value);
        } while (true);
    }

    static __forceinline std::string get_random(size_t length_min, size_t length_max, const char* char_list = __defaultRandomCharList, size_t char_list_size = sizeof(__defaultRandomCharList)) {
        std::string buffer;

        srand(unsigned(::time(0)) * GetCurrentThreadId());

        auto length = length_min;
        if (length_max) {
            length = rand() % (length_max - length_min - 1) + length_min;
        }

        buffer.reserve(length);

        for (length += 1; length >= 1; length--) {
            buffer += char_list[rand() % (char_list_size - 1)];
        }

        return buffer;
    }

    static __forceinline std::vector<std::string> split(const std::string& data, char delim) {
        std::vector<std::string> result;
        std::stringstream stream(data);
        std::string item;

        while (std::getline(stream, item, delim)) {
            result.push_back(item);
        }

        return result;
    }

    static __forceinline const std::string& to_lower(const std::string& data) {
        if (data.empty()) _Exit: return data;

        auto letter = (byte_t*)data.data();
        do *letter = std::tolower(*letter); while (*(letter++));

        goto _Exit;
    }

    template<size_t _bufferSize = 512> static __forceinline std::string format(const char* const data, ...) {
        va_list list;
        va_start(list, data);

        char buffer[_bufferSize]{ 0 };
        vsnprintf(buffer, sizeof(buffer), data, list);

        va_end(list);

        return buffer;
    }
}