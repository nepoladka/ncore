#pragma once
#include "defines.hpp"
#include "handle.hpp"
#include <windows.h>
#include <shlobj.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

namespace ncore::files {
    static __forceinline bool file_exists(const std::string& path) noexcept {
        auto error = std::error_code();
        const auto file_type = std::filesystem::status(path, error).type();
        if (file_type == std::filesystem::file_type::none) return false;
        return file_type != std::filesystem::file_type::not_found;
    }

    static __forceinline bool remove_file(const std::string& path) noexcept {
        return DeleteFileA(path.c_str());
    }

    static __forceinline std::vector<byte_t> read_file(const std::string& path) noexcept {
        std::vector<byte_t> result;

        auto input = std::ifstream(path, std::ios::binary);
        if (!input) _Exit: return result;

        result.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();

        goto _Exit;
    }

    static __forceinline size_t read_file(const std::string& path, byte_t** _result) noexcept {
        if (!_result) _Fail: return 0;

        auto handle = handle::native_handle_t(
            CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
            handle::native_handle_t::closer_t(CloseHandle), true);

        if (!handle || handle == INVALID_HANDLE_VALUE) goto _Fail;

        auto size = GetFileSize(handle.get(), null);
        if (!size) {
        _Exit:
            return size;
        }
        *_result = new byte_t[size]{ null };

        ::ReadFile(handle.get(), *_result, size, nullptr, nullptr);

        goto _Exit;
    }

    static __forceinline bool write_file(const std::string& path, const void* data, size_t size) noexcept {
        std::ofstream output(path, std::ios::binary);
        if (!(output && data && size)) return false;

        output.write((const char*)data, size);
        output.close();

        return true;
    }

    static __forceinline bool write_file(const std::string& path, const std::vector<byte_t>& data) noexcept {
        return write_file(path, data.data(), data.size());
    }

    static __forceinline std::string module_directory(address_t module) noexcept {
        char file_path[MAX_PATH + FILENAME_MAX + 1]{ 0 };
        char file_drive[FILENAME_MAX + 1]{ 0 };
        char file_folder[FILENAME_MAX + 1]{ 0 };

        GetModuleFileNameA(HMODULE(module), file_path, sizeof(file_path));
        _splitpath(file_path, file_drive, file_folder, null, null);
        return std::string(file_drive) + std::string(file_folder);
    }

    static __forceinline std::string current_directory() noexcept {
        static auto path = module_directory(NULL);
        return path;
    }

    static __forceinline std::string system_directory() noexcept {
        static char path[MAX_PATH]{ 0 };
        if (!*path) {
            GetSystemDirectoryA(path, MAX_PATH);

            strcat(path, "\\");
        }
        return path;
    }

    static __forceinline std::string desktop_directory() noexcept {
        static char path[MAX_PATH]{ 0 };
        if (!*path) {
            SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);

            strcat(path, "\\");
        }
        return path;
    }
}
