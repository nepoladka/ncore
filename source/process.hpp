#pragma once
#include "defines.hpp"
#include "handle.hpp"
#include "static_array.hpp"
#include "environment.hpp"
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>

namespace ncore {
    static const handle::native_handle_t::closer_t const __processHandleCloser = handle::native_handle_t::closer_t(NtClose);
    static const handle::native_handle_t::closer_t const __snapshotHandleCloser = handle::native_handle_t::closer_t(NtClose);
    static constexpr const unsigned const __defaultOpenAccess = PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION;

    class process;

    template<unsigned _bufferSize = 1024> static __forceinline std::vector<process> get_processes(unsigned open_access = __defaultOpenAccess);

    class process {
    public:
        class window_t { //todo: move it to single file and do other functions like get_size(), get_position(), capture(), etc.
        public:
            using handle_t = HWND;

            handle_t handle;

            __forceinline window_t(handle_t handle = null) noexcept {
                this->handle = handle;
            }

            static __forceinline window_t get_focused() {
                return GetForegroundWindow();
            }

            template<size_t _bufferSize = 0xff> __forceinline std::string __fastcall get_title() const noexcept {
                char buffer[_bufferSize] = { null };
                GetWindowTextA(handle, buffer, sizeofarr(buffer));
                return buffer;
            }

            __forceinline bool alive() const noexcept {
                return IsWindow(handle);
            }

            __forceinline constexpr bool focused() const noexcept {
                return get_focused() == handle;
            }

            __forceinline bool visible() const noexcept {
                return IsWindowVisible(handle);
            }

            __forceinline constexpr const handle_t& const __fastcall safe_handle() const noexcept {
                return handle;
            }

            __forceinline constexpr bool operator==(handle_t handle) const noexcept {
                return this->handle == handle;
            }

            __forceinline constexpr bool operator==(const window_t& second) const noexcept {
                return this->handle == second.handle;
            }
        };

        class module_t : private MODULEENTRY32 { //todo: move it to single file
        public:
            struct export_t {
                address_t module;

                ui16_t ordinal;
                address_t address;
                static_array<char, 0xff> name;
            };

            __forceinline module_t() : MODULEENTRY32() {
                dwSize = sizeof(MODULEENTRY32);
            }

        private:
            template<typename data_t = const void*> using export_enumeration_callback_t = get_procedure_t(bool, , export_t&, data_t, bool*);

            template<typename data_t = const void*> __forceinline bool enumerate_exports(const process& process, export_enumeration_callback_t<data_t> callback, data_t data) const noexcept {
                auto image_header = process.read_memory<IMAGE_DOS_HEADER>(address_t(modBaseAddr));
                if (image_header.e_magic != IMAGE_DOS_SIGNATURE) _Fail: return false;

                auto nt_headers = process.read_memory<IMAGE_NT_HEADERS>(address_t(modBaseAddr + image_header.e_lfanew));
                if (nt_headers.Signature != IMAGE_NT_SIGNATURE) goto _Fail;

                auto export_directory_offset = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
                if (!export_directory_offset) goto _Fail;

                auto export_directory = process.read_memory<IMAGE_EXPORT_DIRECTORY>(address_t(modBaseAddr + export_directory_offset));

                auto addresses_table = (ui32_t*)(modBaseAddr + export_directory.AddressOfFunctions);
                auto names_table = (ui32_t*)(modBaseAddr + export_directory.AddressOfNames);
                auto ordinals_table = (ui16_t*)(modBaseAddr + export_directory.AddressOfNameOrdinals);

                for (size_t i = 0; i < export_directory.NumberOfNames; i++) {
                    auto name_offset = process.read_memory<ui32_t>(names_table + i);
                    auto export_name = process.read_memory<static_array<char, 0xff>>(modBaseAddr + name_offset);

                    if (export_name.data() != export_name) continue;

                    auto ordinal = process.read_memory<ui16_t>(ordinals_table + i);
                    auto offset = process.read_memory<ui32_t>(addresses_table + ordinal);

                    auto info = export_t{
                        modBaseAddr,
                        ordinal,
                        modBaseAddr + offset,
                        export_name
                    };

                    auto result = false;
                    if (callback(info, data, &result)) return result;
                }

                goto _Fail;
            }

        public:
            __forceinline constexpr MODULEENTRY32& base() const noexcept {
                return *(MODULEENTRY32*)this;
            }

            __forceinline constexpr address_t address() const noexcept {
                return modBaseAddr;
            }

            __forceinline constexpr size_t size() const noexcept {
                return modBaseSize;
            }

            __forceinline constexpr std::string name() const noexcept {
                return szModule;
            }

            __forceinline constexpr std::string path() const noexcept {
                return szExePath;
            }

            __forceinline constexpr id_t process_id() const noexcept {
                return th32ProcessID;
            }

            __forceinline constexpr id_t id() const noexcept {
                return th32ModuleID;
            }

            __forceinline std::vector<export_t> get_exports(const process& process) const noexcept {
                using result_t = std::vector<export_t>;

                static auto callback = [](export_t& info, result_t* _result, bool* _return) noexcept {
                    _result->push_back(info);
                    return !(*_return = true);
                };

                auto result = result_t();
                enumerate_exports<result_t*>(process, callback, &result);
                return result;
            }

            __forceinline export_t search_export(const process& process, const std::string& name) const noexcept {
                struct searching_info {
                    std::string name;
                    export_t result;
                };

                static auto callback = [](export_t& info, searching_info* _result, bool* _return) noexcept {
                    if (*_return = (_result->name == info.name.data())) {
                        _result->result = info;
                    }
                    return *_return;
                };

                auto info = searching_info{
                    name,
                    export_t()
                };

                enumerate_exports<searching_info*>(process, callback, &info);

                return info.result;
            }
        };

        static __forceinline handle::native_t get_handle(id_t id, unsigned access = __defaultOpenAccess) {
            auto result = handle::native_t();
            auto attributes = OBJECT_ATTRIBUTES();
            auto client_id = CLIENT_ID();

            client_id.UniqueThread = handle::native_t(null);
            client_id.UniqueProcess = handle::native_t(id);
            InitializeObjectAttributes(&attributes, null, null, null, null);

            return NT_SUCCESS(NtOpenProcess(&result, access, &attributes, &client_id)) ? result : nullptr;
        }

    private:
        struct string_utils {
            static __forceinline const std::string& to_lower(const std::string& data) {
                if (data.empty()) _Exit: return data;

                auto letter = (byte_t*)data.data();
                do *letter = byte_t(std::tolower(int(*letter))); while (*(letter++));

                goto _Exit;
            }
        };

        using handle_t = handle::native_handle_t;
        using load_library_t = get_procedure_t(address_t, , const char*);
        template<typename data_t = const void*> using module_enumeration_callback_t = get_procedure_t(bool, , module_t&, data_t, address_t*);

        id_t _id;
        handle_t _handle;

        __forceinline handle_t temp_handle(id_t id, const handle_t& source, unsigned open_access) const noexcept {
            handle_t value;
            if (!source) {
                (value = handle_t(get_handle(id, open_access), __processHandleCloser, false)).close_on_destroy(true);
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

        template<typename data_t = const void*> __forceinline address_t enumerate_modules(module_enumeration_callback_t<data_t> callback, data_t data, module_t* _module = nullptr) const noexcept {
            auto snapshot = handle::native_handle_t(
                CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _id),
                __snapshotHandleCloser,
                true);

            if (!snapshot) _Fail: return nullptr;

            auto module = module_t();

            if (!Module32First(snapshot.get(), LPMODULEENTRY32(&module))) goto _Fail;

            do {
                auto result = module.address();
                if (!callback(module, data, &result)) continue;

                if (_module)
                    *_module = module;

                return result;
            } while (Module32Next(snapshot.get(), LPMODULEENTRY32(&module)));

            goto _Fail;
        }

    public:
        __forceinline process(id_t id = null, unsigned open_access = null) noexcept {
            if ((_id = id) && open_access) {
                _handle = handle_t(get_handle(id, open_access), __processHandleCloser, false);
            }
        }

        __forceinline process(handle::native_t win32_handle) noexcept {
            _id = GetProcessId(win32_handle);
            _handle = handle_t(win32_handle, __processHandleCloser, false);
        }

        static __forceinline process current(unsigned open_access = PROCESS_ALL_ACCESS) noexcept {
            return open_access ? process(__process_id, open_access) : process(__current_process);
        }

        static __forceinline process get_by_id(id_t id, unsigned open_access = null) noexcept {
            return process(id, open_access);
        }

        static __forceinline process get_by_name(const std::string& name, unsigned open_access = __defaultOpenAccess) noexcept {
            auto processes = get_processes(open_access);
            for (auto& process : processes) {
                if (process.get_name() == name) return ncore::process(process.id(), open_access);
            }
            return process();
        }

        static __forceinline process get_by_window(HWND window, unsigned open_access = null) noexcept {
            auto owner_id = id_t(null);
            GetWindowThreadProcessId(window, &owner_id);
            return process(owner_id, open_access);
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

        __forceinline handle::native_t handle(unsigned open_access = __defaultOpenAccess) const noexcept {
            return _handle.get() ? _handle.get() : ((*(handle::native_handle_t*)&_handle) = handle::native_handle_t(get_handle(_id, open_access))).get();
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

        template<size_t _bufferSize = MAX_PATH + FILENAME_MAX> __forceinline std::string get_path() const noexcept {
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

        __forceinline std::string get_directory() {
            auto path = get_path();

            char drive[MAX_PATH]{ 0 };
            char folder[MAX_PATH]{ 0 };
            _splitpath(path.c_str(), drive, folder, nullptr, nullptr);

            return drive + std::string(folder);
        }

        __forceinline std::string get_name() {
            auto path = get_path();

            char name[FILENAME_MAX + 1]{ 0 };
            _splitpath(path.c_str(), nullptr, nullptr, name, nullptr);

            return name;
        }

        __forceinline std::string get_extension() {
            auto path = get_path();

            char extension[FILENAME_MAX + 1]{ 0 };
            _splitpath(path.c_str(), nullptr, nullptr, nullptr, extension);

            return extension;
        }

        __forceinline PEB get_environment() const noexcept {
            auto result = PEB();

            auto handle = temp_handle(_id, _handle, __defaultOpenAccess);
            if (!handle) _Exit: return result;

            auto information = PROCESS_BASIC_INFORMATION();
            auto information_length = ULONG(sizeof(information));
            if (NT_ERROR(NtQueryInformationProcess(handle.get(), ProcessBasicInformation, &information, sizeof(PROCESS_BASIC_INFORMATION), &information_length))) goto _Exit;

            auto environment = PEB();
            if (NT_ERROR(NtReadVirtualMemory(handle.get(), information.PebBaseAddress, &environment, sizeof(environment), nullptr))) goto _Exit;

            return environment;
        }

        __forceinline std::string get_command_line() const noexcept {
            auto result = std::string();

            auto handle = temp_handle(_id, _handle, __defaultOpenAccess);
            if (!handle) _Exit: return result;

            auto information = PROCESS_BASIC_INFORMATION();
            auto information_length = ULONG(sizeof(information));
            if (NT_ERROR(NtQueryInformationProcess(handle.get(), ProcessBasicInformation, &information, sizeof(PROCESS_BASIC_INFORMATION), &information_length))) goto _Exit;

            auto environment = PEB();
            if (NT_ERROR(NtReadVirtualMemory(handle.get(), information.PebBaseAddress, &environment, sizeof(environment), nullptr))) goto _Exit;

            auto parameters = RTL_USER_PROCESS_PARAMETERS();
            if (NT_ERROR(NtReadVirtualMemory(handle.get(), environment.ProcessParameters, &parameters, sizeof(parameters), nullptr))) goto _Exit;

            auto length = parameters.CommandLine.MaximumLength;
            auto buffer = new wchar_t[length] { 0 };
            auto result_buffer = new char[length] { 0 };

            if (NT_SUCCESS(NtReadVirtualMemory(handle.get(), parameters.CommandLine.Buffer, buffer, length, nullptr))) {
                WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, buffer, length, result_buffer, length, nullptr, nullptr);
                result = result_buffer;
            }

            delete[] buffer;
            delete[] result_buffer;

            goto _Exit;
        }

        __forceinline address_t get_base() const noexcept {
            return get_environment().ImageBaseAddress;
        }

        __forceinline std::vector<window_t> get_windows() const noexcept {
            auto results = std::vector<window_t>();

            auto current = window_t(null);
            do {
                current.handle = FindWindowExA(null, current.handle, null, null);

                auto owner_id = id_t(null);
                GetWindowThreadProcessId(current.handle, &owner_id);

                if (owner_id == _id) {
                    results.push_back(current);
                }
            } while (current.handle);

            return results;
        }

        __forceinline std::vector<module_t> get_modules() const noexcept {
            using result_t = std::vector<module_t>;

            static auto callback = [](module_t& module, result_t* result, address_t* _return) {
                result->push_back(module);
                return false;
            };

            auto result = result_t();
            enumerate_modules<result_t*>(callback, &result);
            return result;
        }


        __forceinline std::vector<module_t::export_t> search_exports(const std::string& name, size_t max_results_count = 1) const noexcept {
            using result_t = std::vector<module_t::export_t>;

            struct searching_info {
                const process* instance;
                result_t results;
                size_t max_results_count;
                std::string name;
            };
            
            static auto callback = [](module_t& module, searching_info* data, address_t* _return) noexcept {
                auto export_info = module.search_export(*data->instance, data->name);
                if (export_info.module) {
                    if (data->results.size() == data->max_results_count) return true;

                    data->results.push_back(export_info);
                }

                return false;
            };

            auto info = searching_info{
                this,
                result_t(),
                max_results_count ? max_results_count : 1,
                name 
            };

            enumerate_modules<searching_info*>(callback, &info);

            return info.results;
        }

        __forceinline address_t search_module(const std::string& name, module_t* _module = nullptr) const noexcept {
            static auto callback = [](module_t& module, const std::string* name, address_t* _return) noexcept {
                return string_utils::to_lower(module.name()) == *name;
            };

            return enumerate_modules<const std::string*>(callback, &string_utils::to_lower(name), _module);
        }

        __forceinline bool load_library(const std::string& file) noexcept {
            auto result = false;
            if (file.empty()) _Exit: return result;

            if (_id == GetCurrentProcessId()) return LoadLibraryA(file.c_str());

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) goto _Exit;

            auto memory = address_t(nullptr);
            auto size = file.length();
            if (NT_ERROR(NtAllocateVirtualMemory(handle.get(), &memory, null, &size, MEM_COMMIT, PAGE_READWRITE))) goto _Exit;

            if (result = NT_SUCCESS(NtWriteVirtualMemory(handle.get(), memory, address_t(file.c_str()), file.length(), nullptr))) {
                auto library_loading_procedure = load_library_t(search_exports("LoadLibraryA", 1).front().address);

                if (library_loading_procedure) {
                    auto thread = CreateRemoteThread(handle.get(), nullptr, null, LPTHREAD_START_ROUTINE(library_loading_procedure), memory, null, nullptr);
                    WaitForSingleObject(thread, INFINITE);
                }
            }

            NtFreeVirtualMemory(handle.get(), &memory, &size, MEM_DECOMMIT);

            goto _Exit;
        }

        __forceinline address_t allocate_memory(size_t size = PAGE_SIZE, ui32_t protection = PAGE_EXECUTE_READWRITE, address_t address = nullptr) noexcept {
            auto result = address_t(nullptr);

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) _Exit: return result;

            if (NT_SUCCESS(NtAllocateVirtualMemory(handle.get(), &address, null, &size, MEM_COMMIT, protection))) {
                result = address;
            }

            goto _Exit;
        }

        __forceinline bool release_memory(address_t address, size_t size = PAGE_SIZE) noexcept {
            auto result = bool(false);

            if (!(address && size)) _Exit: return result;

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) goto _Exit;

            result = NT_SUCCESS(NtFreeVirtualMemory(handle.get(), &address, &size, MEM_DECOMMIT));

            goto _Exit;
        }

        template<typename _t> __forceinline constexpr bool write_memory(address_t address, const _t& data) noexcept {
            auto result = bool(false);

            if (!address) _Exit: return result;

            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) goto _Exit;

            result = NT_SUCCESS(NtWriteVirtualMemory(handle.get(), address, address_t(&data), sizeof(_t), nullptr));

            goto _Exit;
        }

        template<typename _t> __forceinline constexpr _t read_memory(address_t address) const noexcept {
            auto result = _t();

            if (address) {
                auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
                if (handle.get()) {
                    NtReadVirtualMemory(handle.get(), address, &result, sizeof(_t), nullptr);
                }
            }
            
            return result;
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