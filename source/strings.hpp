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

namespace ncore::strings {
	using string_t = std::string;
	
    static constexpr const char const __defaultRandomCharList[] = {
            "1234567890"
            "QWERTYUIOPASDFGHJKLZXCVBNM"
            "qwertyuiopasdfghjklzxcvbnm"
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

    static __forceinline string_t get_random_string(size_t length_min, size_t length_max, const char* char_list = __defaultRandomCharList, size_t char_list_size = sizeof(__defaultRandomCharList)) {
        string_t buffer;

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

    static __forceinline std::vector<string_t> split_string(const string_t& data, char separator) {
        std::vector<string_t> result;
        std::stringstream stream(data);
        string_t item;

        while (std::getline(stream, item, separator)) {
            result.push_back(item);
        }

        return result;
    }

    static __forceinline string_t string_to_lower(const string_t& data) {
        auto result = string_t(data);
        if (data.empty()) _Exit: return result;

        auto letter = (byte_t*)result.data();
        do *letter = std::tolower(*letter); while (*(letter++));

        goto _Exit;
    }

    static __forceinline const string_t& make_string_lower(const string_t& data) {
        if (data.empty()) _Exit: return data;

        auto letter = (byte_t*)data.data();
        do *letter = std::tolower(*letter); while (*(letter++));

        goto _Exit;
    }

    template<size_t _bufferSize = 512> static __forceinline string_t format_string(const char* const data, ...) {
        va_list list;
        va_start(list, data);

        char buffer[_bufferSize]{ 0 };
        vsnprintf(buffer, sizeof(buffer), data, list);

        va_end(list);

        return buffer;
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