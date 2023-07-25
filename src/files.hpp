#pragma once
#include "defines.hpp"
#include "handle.hpp"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

namespace ncore::files {
    using namespace std;

    static __forceinline std::vector<byte_t> read_file(const string& path) {
        std::vector<byte_t> result;

        auto input = ifstream(path, ios::binary);
        if (!input) _Exit: return result;

        result.assign((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
        input.close();

        goto _Exit;
    }

    static __forceinline size_t read_file(const string& path, byte_t** _result) {
        if (!_result) _Fail: return 0;

        auto handle = ncore::handle::native_handle_t(
            CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
            (ncore::handle::native_handle_t::closer_t)CloseHandle, true);

        if (!handle || handle == INVALID_HANDLE_VALUE) goto _Fail;

        auto size = GetFileSize(handle.get(), NULL);
        if (!size)
        {
        _Exit:
            return size;
        }
        *_result = new byte_t[size]{ NULL };

        ::ReadFile(handle.get(), *_result, size, NULL, FALSE);

        goto _Exit;
    }

    static __forceinline bool write_file(const string& path, const void* data, size_t size) {
        ofstream output(path, ios::binary);
        if (!(output && data && size)) return false;

        output.write((const char*)data, size);
        output.close();

        return true;
    }

    static __forceinline bool write_file(const string& path, const vector<byte_t>& data) {
        return write_file(path, data.data(), data.size());
    }

    static __forceinline string module_directory(HMODULE module) {
        char file_path[MAX_PATH + FILENAME_MAX + 1]{ 0 };
        char file_drive[FILENAME_MAX + 1]{ 0 };
        char file_folder[FILENAME_MAX + 1]{ 0 };

        GetModuleFileNameA(module, file_path, sizeof(file_path));
        _splitpath(file_path, file_drive, file_folder, NULL, NULL);
        return string(file_drive) + string(file_folder);
    }

    static __forceinline string current_directory() {
        static auto path = module_directory(NULL);

        return path;
    }

    static __forceinline string system_directory() {
        static char path[MAX_PATH]{ 0 };
        if (!*path) {
            GetSystemDirectoryA(path, MAX_PATH);

            strcat(path, "\\");
        }
        return path;
    }

    static __forceinline string desktop_directory() {
        static char path[MAX_PATH]{ 0 };
        if (!*path) {
            SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);

            strcat(path, "\\");
        }
        return path;
    }
}
