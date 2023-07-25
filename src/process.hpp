#pragma once
#include "handle.hpp"
#include "defines.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include "includes/ntos.h"

namespace ncore {
    using namespace std;

    static const handle::native_handle_t::closer_t const __processHandleCloser = handle::native_handle_t::closer_t(NtClose);
    static const handle::native_handle_t::closer_t const __snapshotHandleCloser = handle::native_handle_t::closer_t(NtClose);

    class process;

    template<unsigned _bufferSize = 1024> static __forceinline std::vector<process> get_processes(unsigned open_access = PROCESS_ALL_ACCESS);

    class process {
    private:
        using handle_t = handle::native_handle_t;
        using window_t = HWND;
        using module_t = MODULEENTRY32;
        using module_enumeration_callback_t = get_procedure_t(bool, , module_t&, const void*, address_t*);
        using load_library_t = get_procedure_t(address_t, , const char*);

        id_t _id;
        handle_t _handle;

        __forceinline handle_t temp_handle(id_t id, const handle_t& source, unsigned open_access) const noexcept {
            handle_t value;
            if (!source) {
                (value = handle_t(OpenProcess(open_access, false, id), __processHandleCloser, false)).close_on_destroy(true);
            }
            else {
                value = source;
            }

            return value;
        }

        __forceinline bool set_suspended(bool state) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, PROCESS_SUSPEND_RESUME);
            if (!handle) {
            _Exit:
                return result;
            }

            auto procedure = state ? NtSuspendProcess : NtResumeProcess;
            result = NT_SUCCESS(procedure(handle.get()));

            goto _Exit;
        }

        struct string_utils {
            static __forceinline const std::string& to_lower(const std::string& data) {
                if (data.empty()) _Exit: return data;

                auto letter = (byte_t*)data.data();
                do *letter = std::tolower(*letter); while (*(letter++));

                goto _Exit;
            }
        };

    public:
        __forceinline process(id_t id = null, unsigned open_access = null) noexcept {
            if ((_id = id) && open_access) {
                _handle = handle_t(OpenProcess(id, false, open_access), __processHandleCloser, false);
            }
        }

        __forceinline process(handle::native_t win32_handle) noexcept {
            _id = GetProcessId(win32_handle);
            _handle = handle_t(win32_handle, __processHandleCloser, false);
        }

        static __forceinline process current() noexcept {
            return process(GetCurrentProcessId());
        }

        static __forceinline process get_by_id(id_t id) noexcept {
            return process(id);
        }

        static __forceinline process get_by_name(std::string name, unsigned access = PROCESS_QUERY_LIMITED_INFORMATION) noexcept {
            auto processes = get_processes(access);
            for (auto& process : processes) {
                if (process.get_name() == name) return process;
            }
            return process();
        }

        static __forceinline process get_by_window(HWND window) noexcept {
            auto owner_id = id_t(null);
            GetWindowThreadProcessId(window, &owner_id);
            return process(owner_id);
        }

        static __forceinline process create(const std::string& file_path, const std::string& command_line = std::string(), const std::string& verbose = std::string(), bool keep_handle = false) noexcept {
            if (file_path.empty()) _Fail: return process();

            auto execution_info = SHELLEXECUTEINFOA{
                        DWORD32(sizeof(SHELLEXECUTEINFOA)),
                        ULONG(SEE_MASK_NOCLOSEPROCESS),
                        HWND(NULL),
                        LPCSTR(verbose.empty() ? nullptr : verbose.c_str()),
                        LPCSTR(file_path.c_str()),
                        LPCSTR(command_line.empty() ? nullptr : command_line.c_str()),
                        LPCSTR(NULL),
                        int(SW_NORMAL) };

            if (!ShellExecuteExA(&execution_info)) goto _Fail;

            if (keep_handle) return process(execution_info.hProcess);
            
            auto id = id_t(GetProcessId(execution_info.hProcess));
            __processHandleCloser(execution_info.hProcess);
            
            return process(id);
        }


        __forceinline id_t id() const noexcept {
            return _id;
        }

        __forceinline handle::native_t handle() const noexcept {
            return _handle.get();
        }

        __forceinline void close_handle() noexcept {
            return _handle.close();
        }

        __forceinline unsigned wait() const noexcept {
            auto result = 0ui32;

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) {
            _Exit:
                return result;
            }

            result = WaitForSingleObject(handle.get(), INFINITE);

            goto _Exit;
        }

        __forceinline bool alive(unsigned* _exit_code = nullptr) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            result = (WaitForSingleObject(handle.get(), null) != WAIT_OBJECT_0);

            if (_exit_code) {
                GetExitCodeProcess(handle.get(), LPDWORD(_exit_code));
            }

            goto _Exit;
        }

        __forceinline bool suspend() const noexcept {
            return set_suspended(true);
        }

        __forceinline bool resume() const noexcept {
            return set_suspended(false);
        }

        __forceinline bool terminate(long exit_status = EXIT_SUCCESS) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, PROCESS_TERMINATE);
            if (!handle) {
            _Exit:
                return result;
            }

            result = NT_SUCCESS(NtTerminateProcess(handle.get(), exit_status));

            goto _Exit;
        }

        __forceinline bool set_priority(int priority_class) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) {
            _Exit:
                return result;
            }

            result = SetPriorityClass(handle.get(), priority_class);

            goto _Exit;
        }

        __forceinline int get_priority() const noexcept {
            auto result = null;

            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            result = GetPriorityClass(handle.get());

            goto _Exit;
        }


        template<size_t _bufferSize = MAX_PATH + FILENAME_MAX> __forceinline std::string get_path() {
            auto result = std::string();

            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            unsigned long buffer_size = _bufferSize;
            char buffer[_bufferSize] = { null };

            if (QueryFullProcessImageNameA(handle.get(), null, buffer, &buffer_size)) {
                result = buffer;
            }

            goto _Exit;
        }

        __forceinline std::string get_name() {
            auto path = get_path();

            char name[FILENAME_MAX + 1]{ 0 };
            _splitpath(path.c_str(), nullptr, nullptr, name, nullptr);

            return name;
        }

        __forceinline std::vector<window_t> get_windows() {
            auto results = std::vector<window_t>();

            auto current = window_t(null);
            do {
                current = FindWindowExA(null, current, null, null);

                auto owner_id = id_t(null);
                GetWindowThreadProcessId(current, &owner_id);

                if (owner_id == _id) {
                    results.push_back(current);
                }
            } while (current);

            return results;
        }

        __forceinline address_t enumerate_modules(module_enumeration_callback_t callback, const void* data, module_t* _module = nullptr) {
            auto snapshot = handle::native_handle_t(
                CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _id),
                __snapshotHandleCloser,
                true);

            if (!snapshot) _Fail: return nullptr;

            auto module = module_t{ 0 };
            module.dwSize = sizeof(module_t);

            if (!Module32First(snapshot.get(), &module)) goto _Fail;

            do {
                auto return_value = address_t(module.hModule);
                if (!callback(module, data, &return_value)) continue;

                if (_module)
                    *_module = module;

                return return_value;
            } while (Module32Next(snapshot.get(), &module));

            goto _Fail;
        }

        __forceinline address_t search_export(const std::string& name, module_t* _module = nullptr) {
            static auto callback = [](module_t& module, const void* data, address_t* _return) {
                return bool(*_return = GetProcAddress(module.hModule, (char*)data));
            };

            return enumerate_modules(callback, name.c_str(), _module);
        }

        __forceinline address_t search_module(const std::string& name, module_t* _module = nullptr) {
            static auto callback = [](module_t& module, const void* data, address_t* _return) {
                return string_utils::to_lower(module.szModule) == *(std::string*)data;
            };

            return enumerate_modules(callback, &string_utils::to_lower(name), _module);
        }

        __forceinline bool load_library(const std::string& file) {
            auto result = false;
            if (file.empty()) _Exit: return result;

            if (_id == GetCurrentProcessId()) return LoadLibraryA(file.c_str());

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) goto _Exit;

            auto memory = address_t(nullptr);
            auto size = file.length();
            if (NT_ERROR(NtAllocateVirtualMemory(handle.get(), &memory, null, &size, MEM_COMMIT, PAGE_READWRITE))) goto _Exit;

            if (result = NT_SUCCESS(NtWriteVirtualMemory(handle.get(), memory, address_t(file.c_str()), file.length(), nullptr))) {
                auto library_loading_procedure = load_library_t(search_export("LoadLibraryA"));

                if (library_loading_procedure) {
                    auto thread = CreateRemoteThread(handle.get(), nullptr, null, LPTHREAD_START_ROUTINE(library_loading_procedure), memory, null, nullptr);
                    WaitForSingleObject(thread, INFINITE);
                }
            }

            NtFreeVirtualMemory(handle.get(), &memory, &size, MEM_DECOMMIT);

            goto _Exit;
        }
    };

    template<unsigned _bufferSize> static __forceinline std::vector<process> get_processes(unsigned open_access)
    {
        std::vector<process> results;

        id_t buffer[_bufferSize] = { null };
        unsigned long count = null;

        if (!K32EnumProcesses(buffer, sizeof(id_t) * _bufferSize, &count)) _Exit: return results;
        count /= sizeof(id_t);

        for (unsigned i = null; i < count; i++) {
            auto handle = OpenProcess(open_access, FALSE, buffer[i]);
            if (!handle) continue;

            CloseHandle(handle);

            results.push_back(process(buffer[i]));
        }

        goto _Exit;
    }

}