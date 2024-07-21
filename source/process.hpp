#pragma once
#include "thread.hpp"
#include "enumeration.hpp"
#include "file.hpp"

#include <tlhelp32.h>
#include <psapi.h>
#include <vector>

#ifndef NCORE_PROCESS_NO_MINIDUMP
#include <minidumpapiset.h>
#include <mindumpdef.h>
#pragma comment(lib, "dbghelp.lib")
#endif

#ifdef NCORE_PROCESS_EXTRA
NCORE_PROCESS_EXTRA
#endif

#pragma warning(disable : 4996)

namespace ncore {
    static const auto const __processHandleCloser = handle::native_handle_t::closer_t(NtClose);
    static const auto const __snapshotHandleCloser = handle::native_handle_t::closer_t(NtClose);
    static constexpr const auto const __defaultProcessOpenAccess = ui32_t(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_SUSPEND_RESUME | PROCESS_QUERY_INFORMATION);

    class process;

    template<unsigned _bufferSize = 1024> static __forceinline std::vector<process> get_processes(unsigned open_access = __defaultProcessOpenAccess);

    class process {
    private:
        using handle_t = handle::native_handle_t;
        using load_library_t = get_procedure_t(address_t, , const char*);

    public:
        class memory_t : private MEMORY_BASIC_INFORMATION {
        public:
            using base_t = MEMORY_BASIC_INFORMATION;

            __forceinline memory_t() = default;

            __forceinline memory_t(const base_t& info) noexcept : base_t(info) {
                return;
            }

            __forceinline constexpr const auto& base() const noexcept {
                return *((base_t*)this);
            }

            __forceinline constexpr auto& base() noexcept {
                return *((base_t*)this);
            }

            __forceinline constexpr auto region() const noexcept {
                return address_t(this->AllocationBase);
            }

            __forceinline constexpr auto address() const noexcept {
                return address_t(this->BaseAddress);
            }

            __forceinline constexpr auto size() const noexcept {
                return size_t(this->RegionSize);
            }
        };

        class region_t : private MEMORY_REGION_INFORMATION {
        private:
            std::vector<memory_t> _allocations;

        public:
            using base_t = MEMORY_REGION_INFORMATION;

            __forceinline region_t() = default;

            __forceinline region_t(const base_t& info, const decltype(_allocations)& memory = {}) noexcept : base_t(info), _allocations(memory) {
                return;
            }

            __forceinline constexpr const auto& base() const noexcept {
                return *((base_t*)this);
            }

            __forceinline constexpr auto& base() noexcept {
                return *((base_t*)this);
            }

            __forceinline constexpr auto address() const noexcept {
                return address_t(this->AllocationBase);
            }

            __forceinline constexpr auto size() const noexcept {
                return size_t(this->RegionSize);
            }

            __forceinline constexpr auto bounds() const noexcept {
                return limit<address_t, true, false>(address(), address_t(ui64_t(address()) + size()));
            }

            __forceinline constexpr const auto& memory() const noexcept {
                return _allocations;
            }

            __forceinline constexpr auto& memory() noexcept {
                return _allocations;
            }
        };

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
                auto buffer = static_array<wchar_t, _bufferSize>();
                auto title = static_array<char, _bufferSize>();

                if (GetWindowTextW(handle, buffer.data(), sizeofarr(buffer))) {
                    u16tou8(buffer.data(), sizeofarr(buffer), title.data(), sizeofarr(title));
                }

                return title.data();
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

        class module_t { //todo: move it to single file 
        public:
            using flags_t = decltype(_LDR_DATA_TABLE_ENTRY_COMPATIBLE::ENTRYFLAGSUNION);

            struct export_t {
                address_t module;

                ui16_t ordinal;
                address_t address;
                static_array<char, 0xff> name;

                __forceinline constexpr offset_t offset() const noexcept {
                    return ui64_t(address) - ui64_t(module);
                }
            };

            __forceinline module_t() = default;

            __forceinline __fastcall module_t(id_t process, address_t image, address_t entry, size_t size, ui16_t tls, flags_t flags, char* path, char* name) noexcept {
                _process_id = process;
                _address = image;
                _entry_point = entry;
                _size = size;
                _tls_index = tls;
                _flags = flags;
                _path = path;
                _name = name;
            }

        protected:
            id_t _process_id;

            address_t _address, _entry_point;
            size_t _size;
            ui16_t _tls_index;
            flags_t _flags;

            static_array<char, 0xff> _path, _name;

            template<typename data_t = const void*> using export_enumeration_callback_t = get_procedure_t(bool, , export_t&, data_t, bool*);

            template<typename data_t = const void*> __forceinline bool enumerate_exports(export_enumeration_callback_t<data_t> callback, data_t data) const noexcept {
                auto process = this->process();

                auto image_base = address<ui64_t>();
                auto image = process.read_memory<IMAGE_DOS_HEADER>(address_t(image_base));
                if (image.e_magic != IMAGE_DOS_SIGNATURE) _Fail: return false;

                auto headers = process.read_memory<IMAGE_NT_HEADERS>(address_t(image_base + image.e_lfanew));
                if (headers.Signature != IMAGE_NT_SIGNATURE) goto _Fail;

                auto export_directory_offset = headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
                if (!export_directory_offset) goto _Fail;

                auto export_directory = process.read_memory<IMAGE_EXPORT_DIRECTORY>(address_t(image_base + export_directory_offset));

                auto addresses_table = (ui32_t*)(image_base + export_directory.AddressOfFunctions);
                auto names_table = (ui32_t*)(image_base + export_directory.AddressOfNames);
                auto ordinals_table = (ui16_t*)(image_base + export_directory.AddressOfNameOrdinals);

                for (size_t i = 0; i < export_directory.NumberOfNames; i++) {
                    auto name_offset = process.read_memory<ui32_t>(names_table + i);
                    auto export_name = process.read_memory<static_array<char, 0xff>>(address_t(image_base + name_offset));

                    //if (export_name.data() != export_name) continue; //17.06.24 - WHAT THA HELL?????????? todo: debug it

                    auto ordinal = process.read_memory<ui16_t>(ordinals_table + i);
                    auto offset = process.read_memory<ui32_t>(addresses_table + ordinal);

                    auto info = export_t{
                        address_t(image_base),
                        ordinal,
                        address_t(image_base + offset),
                        export_name
                    };

                    auto result = false;
                    if (callback(info, data, &result)) return result;
                }

                goto _Fail;
            }

        public:
            template<typename _t = address_t> __forceinline constexpr auto address() const noexcept {
                return _t(_address);
            }

            template<typename _t = address_t> __forceinline constexpr auto entry() const noexcept {
                return _t(_entry_point);
            }

            __forceinline constexpr auto size() const noexcept {
                return _size;
            }

            __forceinline constexpr auto tls_index() const noexcept {
                return _tls_index;
            }

            __forceinline constexpr auto path() const noexcept {
                return std::string(_path.data());
            }

            __forceinline constexpr auto name() const noexcept {
                return std::string(_name.data());
            }

            __forceinline constexpr auto flags() const noexcept {
                return _flags;
            }

            __forceinline auto process() const noexcept {
                return ncore::process(_process_id);
            }

            __forceinline auto get_exports() const noexcept {
                auto result = std::vector<export_t>();

                static auto callback = [](export_t& info, decltype(result)* _result, bool* _return) noexcept {
                    _result->push_back(info);
                    return !(*_return = true);
                };

                enumerate_exports<decltype(result)*>(callback, &result);
                return result;
            }

            __forceinline auto search_export(const std::string& name) const noexcept {
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

                enumerate_exports<searching_info*>(callback, &info);

                return info.result;
            }

            __forceinline auto search_export(const ui16_t ordinal) const noexcept {
                struct searching_info {
                    ui16_t ordinal;
                    export_t result;
                };

                static auto callback = [](export_t& info, searching_info* _result, bool* _return) noexcept {
                    if (*_return = (_result->ordinal == info.ordinal)) {
                        _result->result = info;
                    }
                    return *_return;
                };

                auto info = searching_info{
                    ordinal,
                    export_t()
                };

                enumerate_exports<searching_info*>(callback, &info);

                return info.result;
            }
        };

#ifndef NCORE_PROCESS_NO_MINIDUMP
        class minidump {
        public:
            struct memory_info_t {
            private:
                address_t _address;
                bound<ui64_t> _virtual_bounds;

            public:
                __forceinline constexpr memory_info_t(address_t address = nullptr, ui64_t base = null, size_t size = null) noexcept {
                    _address = address;
                    _virtual_bounds = { base, base + size };
                }

                __forceinline constexpr auto valid() const noexcept {
                    return _address != nullptr;
                }

                __forceinline constexpr auto address() const noexcept {
                    return ui64_t(_virtual_bounds.min);
                }

                __forceinline constexpr auto size() const noexcept {
                    return size_t(_virtual_bounds.max - _virtual_bounds.min);
                }

                template<typename _t = byte_t> __forceinline constexpr auto data() const noexcept {
                    return ((_t*)_address);
                }

                __forceinline constexpr auto bounds() const noexcept {
                    return _virtual_bounds;
                }

                template<typename _t = byte_t> __forceinline constexpr auto begin() const noexcept {
                    return data<_t>();
                }

                template<typename _t = byte_t> __forceinline constexpr auto end() const noexcept {
                    return ((_t*)(data<byte_t>() + size()));
                }
            };

            struct region_info_t {
                address_t address;
                ui64_t virtual_address;

                size_t size;
                std::vector<memory_info_t> allocations;
            };

            struct module_info_t {
            private:
                std::string _name;

                ui64_t _base;
                size_t _size;

            public:
                __forceinline constexpr module_info_t(const std::string& name = std::string(), ui64_t base = null, size_t size = null) noexcept {
                    _name = strings::string_to_lower(name);
                    _base = base;
                    _size = size;
                }

                __forceinline constexpr auto valid() const noexcept {
                    return _base != null;
                }

                __forceinline constexpr auto& name() const noexcept {
                    return _name;
                }

                __forceinline constexpr auto base() const noexcept {
                    return _base;
                }

                __forceinline constexpr auto address() const noexcept {
                    return _base;
                }

                __forceinline constexpr auto size() const noexcept {
                    return _size;
                }

                __forceinline constexpr auto bounds() const noexcept {
                    return bound<ui64_t>(_base, _base + _size);
                }
            };

            struct thread_info_t {
            private:
                id_t _id;
                thread::context_t _context;
                ui64_t _environment;
                ui64_t _stack;

            public:
                __forceinline constexpr thread_info_t(id_t id = {}, const thread::context_t& context = {}, ui64_t environment = {}, ui64_t stack = {}) noexcept {
                    _id = id;
                    _context = context;
                    _environment = environment;
                    _stack = stack;
                }

                __forceinline constexpr auto id() const noexcept {
                    return _id;
                }

                __forceinline constexpr const auto& context() const noexcept {
                    return _context;
                }

                __forceinline constexpr const auto& environment() const noexcept {
                    return _environment;
                }

                __forceinline constexpr const auto& stack() const noexcept {
                    return _stack;
                }
            };

            template<typename data_t = void> using memory_enumeration_procedure_t = get_procedure_t(enumeration::return_t, , const index_t, const memory_info_t&, data_t*);
            template<typename data_t = void> using module_enumeration_procedure_t = get_procedure_t(enumeration::return_t, , const index_t, const module_info_t&, data_t*);
            template<typename data_t = void> using thread_enumeration_procedure_t = get_procedure_t(enumeration::return_t, , const index_t, const thread_info_t&, data_t*);

        private:
            handle::native_t _file = { };
            handle::native_t _mapping = { };
            address_t _data = { };

        public:
            __forceinline minidump() = default;

            __forceinline minidump(const std::string& path) noexcept {
                _file = ncore::file::get_handle(path, FILE_READ_DATA | SYNCHRONIZE);
                if (!_file) _Exit: return;

                _mapping = CreateFileMappingA(_file, nullptr, PAGE_READONLY, null, null, nullptr);
                if (!_mapping) {
                _ReleaseFileAndExit:
                    __fileHandleCloser(_file);
                    _file = nullptr;

                    goto _Exit;
                }

                _data = MapViewOfFile(_mapping, FILE_MAP_READ, null, null, null);
                if (!_data) {
                    __fileHandleCloser(_mapping);
                    _mapping = nullptr;

                    goto _ReleaseFileAndExit;
                }
            }

            __forceinline ~minidump() noexcept {
                if (_file) {
                    __fileHandleCloser(_file);
                    _file = nullptr;
                }

                if (_mapping) {
                    __fileHandleCloser(_mapping);
                    _mapping = nullptr;
                }

                if (_data) {
                    UnmapViewOfFile(_data);
                    _data = nullptr;
                }
            }

            template<typename data_t = void> __forceinline constexpr bool enumerate_modules(module_enumeration_procedure_t<data_t> procedure, data_t* data) const noexcept {
                if (!_data || !procedure) return false;

                auto directory = PMINIDUMP_DIRECTORY(nullptr);
                auto list = PMINIDUMP_MODULE_LIST(nullptr);

                if (!MiniDumpReadDumpStream(_data, MINIDUMP_STREAM_TYPE::ModuleListStream, &directory, address_p(&list), nullptr)) return false;

                auto result = false;

                for (auto i = index_t(0), j = count_t(list->NumberOfModules); i < j; i++) {
                    auto current = list->Modules + i;
                    
                    auto path = PMINIDUMP_STRING(byte_p(_data) + current->ModuleNameRva);

                    auto name = ncore::compatible_string();
                    ncore::path(std::wstring(path->Buffer, path->Length)).parts(nullptr, &name);

                    auto info = module_info_t(name.string(), current->BaseOfImage, current->SizeOfImage);
                    if (result = (procedure(i, info, data) == enumeration::return_t::stop)) break;
                }

                return result;
            }

            template<typename data_t = void> __forceinline constexpr bool enumerate_memory(memory_enumeration_procedure_t<data_t> procedure, data_t* data) const noexcept {
                if (!_data || !procedure) return false;

                auto directory = PMINIDUMP_DIRECTORY(nullptr);
                auto list = PMINIDUMP_MEMORY64_LIST(nullptr);
                
                if (!MiniDumpReadDumpStream(_data, MINIDUMP_STREAM_TYPE::Memory64ListStream, &directory, address_p(&list), nullptr)) return false;

                for (auto i = index_t(0), j = count_t(list->NumberOfMemoryRanges), o = offset_t(list->BaseRva); i < j; i++) {
                    auto current = list->MemoryRanges + i;

                    auto info = memory_info_t(byte_p(_data) + o, current->StartOfMemoryRange, current->DataSize);
                    if (procedure(i, info, data) == enumeration::return_t::stop) return true;

                    o += current->DataSize;
                }

                return false;
            }
            
            template<typename data_t = void> __forceinline constexpr bool enumerate_threads(thread_enumeration_procedure_t<data_t> procedure, data_t* data) {
                if (!_data || !procedure) return false;

                auto directory = PMINIDUMP_DIRECTORY(nullptr);
                auto list = PMINIDUMP_THREAD_LIST(nullptr);

                if (!MiniDumpReadDumpStream(_data, MINIDUMP_STREAM_TYPE::ThreadListStream, &directory, address_p(&list), nullptr)) return false;

                for (auto i = index_t(), j = count_t(list->NumberOfThreads); i < j; i++) {
                    auto current = list->Threads + i;

                    auto info = thread_info_t(
                        current->ThreadId, 
                        *(thread::context_t*)(ui64_t(_data) + current->ThreadContext.Rva),
                        current->Teb,
                        current->Stack.StartOfMemoryRange);

                    if (procedure(i, info, data) == enumeration::return_t::stop) return true;
                }

                return false;
            }

            __forceinline constexpr auto valid() const noexcept {
                return _data != nullptr;
            }

            __forceinline constexpr auto get_modules() const noexcept {
                auto results = std::vector<module_info_t>();

                constexpr const auto procedure = [](const index_t, const module_info_t& module, decltype(results)* _results) noexcept {
                    _results->push_back(module);
                    return enumeration::return_t::next;
                };

                enumerate_modules<decltype(results)>(procedure, &results);

                return results;
            }

            __forceinline constexpr auto get_module(const std::string& name) const noexcept {
                auto result = module_info_t(name);

                constexpr const auto procedure = [](const index_t, const module_info_t& module, decltype(result)* _result) noexcept {
                    if (_result->name() == module.name()) {
                        *_result = module;
                        return enumeration::return_t::stop;
                    }

                    return enumeration::return_t::next;
                    };

                enumerate_modules<decltype(result)>(procedure, &result);

                return result;
            }

            __forceinline constexpr auto get_module(const ui64_t address) const noexcept {
                auto result = module_info_t({}, address);

                constexpr const auto procedure = [](const index_t, const module_info_t& module, decltype(result)* _result) noexcept {
                    if (module.bounds().in_range(_result->base())) {
                        *_result = module;
                        return enumeration::return_t::stop;
                    }

                    return enumeration::return_t::next;
                };

                enumerate_modules<decltype(result)>(procedure, &result);

                return result;
            }

            __forceinline constexpr auto get_memory() const noexcept {
                auto results = std::vector<memory_info_t>();

                constexpr const auto procedure = [](const index_t, const memory_info_t& memory, decltype(results)* _results) noexcept {
                    _results->push_back(memory);
                    return enumeration::return_t::next;
                    };

                enumerate_memory<decltype(results)>(procedure, &results);

                return results;
            }

            __forceinline constexpr auto get_memory(ui64_t address) const noexcept {
                auto result = memory_info_t(nullptr, address);

                constexpr const auto procedure = [](const index_t, const memory_info_t& memory, decltype(result)* _result) noexcept {
                    if (memory.bounds().in_range(_result->address())) {
                        *_result = memory;
                        return enumeration::return_t::stop;
                    }

                    return enumeration::return_t::next;
                    };

                enumerate_memory<decltype(result)>(procedure, &result);

                return result;
            }

            __forceinline constexpr auto read_memory(ui64_t address, size_t size, void* _buffer) const noexcept {
                if (!address || !size || !_buffer) return false;

                auto info = get_memory(address);
                return info.valid() ? bool(memcpy(_buffer, info.begin() + (address - info.address()), size)) : false;
            }

            template<typename _t> __forceinline constexpr auto read_memory(ui64_t address) const noexcept {
                auto buffer = _t();
                read_memory(address, sizeof(_t), &buffer);
                return buffer;
            }

            __forceinline auto get_command_line() const noexcept {
                constexpr const auto bad_result = []() { return compatible_string(); };

                auto directory = PMINIDUMP_DIRECTORY(nullptr);
                auto list = PMINIDUMP_THREAD_LIST(nullptr);

                if (!MiniDumpReadDumpStream(_data, MINIDUMP_STREAM_TYPE::ThreadListStream, &directory, address_p(&list), nullptr)) return bad_result();

                auto teb = TEB();
                if (!read_memory(list->Threads->Teb, sizeof(teb), &teb)) return bad_result();
                
                auto peb = PEB();
                if (!read_memory(ui64_t(teb.ProcessEnvironmentBlock), sizeof(peb), &peb)) return bad_result();

                auto parameters = RTL_USER_PROCESS_PARAMETERS();
                if (!read_memory(ui64_t(peb.ProcessParameters), sizeof(parameters), &parameters)) return bad_result();

                auto length = parameters.CommandLine.Length;
                auto buffer = new wchar_t[parameters.CommandLine.MaximumLength] { 0 };

                if (!read_memory(ui64_t(parameters.CommandLine.Buffer), length, buffer)) return bad_result();

                auto result = compatible_string({ buffer, length });

                delete[] buffer;

                return result;
            }
        };
#endif

        static __forceinline handle::native_t get_handle(id_t id, ui32_t access = __defaultProcessOpenAccess) {
            auto result = handle::native_t();
            auto attributes = OBJECT_ATTRIBUTES();
            auto client_id = CLIENT_ID();

            client_id.UniqueThread = handle::native_t(null);
            client_id.UniqueProcess = handle::native_t(id);
            InitializeObjectAttributes(&attributes, null, null, null, null);

            return NT_SUCCESS(NtOpenProcess(&result, access, &attributes, &client_id)) ? result : nullptr;
        }

    private:
        template<typename data_t = const void*> using module_enumeration_callback_t = get_procedure_t(bool, , module_t&, data_t, address_t*);
        template<typename data_t = const void*> using memory_enumeration_callback_t = get_procedure_t(bool, , handle_t&, memory_t&, data_t, address_t*);

        id_t _id;
        handle_t _handle;

        static __forceinline handle_t temp_handle(id_t id, const handle_t& source, unsigned open_access) noexcept {
            auto value = source;
            if (!value) {
                (value = handle_t(get_handle(id, open_access), __processHandleCloser, false)).close_on_destroy(true);
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
            auto ldr = read_memory<PEB_LDR_DATA>(get_environment().Ldr);

            auto head = ldr.InLoadOrderModuleList.Flink;
            auto current = head;
            do {
                auto entry = read_memory<LDR_DATA_TABLE_ENTRY>(current);
                current = entry.InLoadOrderLinks.Flink;

                if (!entry.DllBase) continue;

                auto buffer = static_array<wchar_t, 0xff>();

                auto path = static_array<char, 0xff>();
                auto name = static_array<char, 0xff>();

                if (entry.FullDllName.Length) {
                    read_memory(entry.FullDllName.Buffer, entry.FullDllName.MaximumLength, buffer.data());

                    u16tou8(buffer.data(), sizeofarr(buffer), path.data(), sizeofarr(path));
                }

                if (entry.BaseDllName.Length) {
                    read_memory(entry.BaseDllName.Buffer, entry.BaseDllName.MaximumLength, buffer.data());

                    u16tou8(buffer.data(), sizeofarr(buffer), name.data(), sizeofarr(name));
                }

                auto module = module_t(_id, entry.DllBase, entry.EntryPoint, entry.SizeOfImage, entry.TlsIndex, entry.ENTRYFLAGSUNION, path.data(), name.data());

                auto result = module.address();
                if (!callback(module, data, &result)) continue;

                if (_module) {
                    *_module = module;
                }

                return result;
            } while (head != current);

            return nullptr;
        }

        template<typename data_t = const void*> __forceinline address_t enumerate_memory(memory_enumeration_callback_t<data_t> callback, data_t data, memory_t* _memory = nullptr) const noexcept {
            auto current = address_t();
            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION);
            if (!handle) return nullptr;

            do {
                auto memory = memory_t();
                if (NT_ERROR(NtQueryVirtualMemory(handle.get(), current, MEMORY_INFORMATION_CLASS::MemoryBasicInformation, &memory, sizeof(memory_t::base_t), nullptr))) break;

                auto result = current;
                if (!callback(handle, memory, data, &result)) {
                    *ui64_p(&current) += memory.size() ? memory.size() : PAGE_SIZE;
                    continue;
                }

                if (_memory) {
                    *_memory = memory;
                }

                return result;
            } while (true);

            return nullptr;
        }

    public:
        __forceinline constexpr process(id_t id = null, unsigned open_access = null) noexcept {
            if ((_id = id) && open_access) {
                _handle = handle_t(get_handle(id, open_access), __processHandleCloser, false);
            }
        }

        __forceinline process(handle::native_t win32_handle) noexcept {
            _id = GetProcessId(win32_handle);
            _handle = handle_t(win32_handle, __processHandleCloser, false);
        }

        static __forceinline process current(ui32_t open_access = __defaultProcessOpenAccess) noexcept {
            return open_access ? process(__process_id, open_access) : process(__current_process);
        }

        static __forceinline process get_by_id(id_t id, ui32_t open_access = null) noexcept {
            return process(id, open_access);
        }

        //executable name without exe [application.exe -> application]
        static __forceinline process get_by_name(const std::string& name, ui32_t open_access = __defaultProcessOpenAccess, get_procedure_t(bool, comparsion_procedure, const std::string& current, const std::string& target) = nullptr) noexcept {
            auto processes = get_processes(open_access);
            for (auto& process : processes) {
                auto current = process.get_name();

                if (comparsion_procedure) {
                    if (comparsion_procedure(current, name)) _End: return ncore::process(process.id(), open_access);
                }
                else if (current == name) goto _End;
            }
            return process();
        }

        static __forceinline process get_by_window(HWND window, ui32_t open_access = null) noexcept {
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

        __forceinline const auto id() const noexcept {
            return _id;
        }

        __forceinline auto handle() const noexcept {
            return _handle.get();
        }

        __forceinline auto handle(ui32_t open_access = __defaultProcessOpenAccess) noexcept {
            return _handle.get() ? _handle.get() : (_handle = handle::native_handle_t(get_handle(_id, open_access), __processHandleCloser)).get();
        }

        __forceinline auto close_handle() noexcept {
            return _handle.close();
        }

        __forceinline auto release() noexcept {
            return _handle.close();
        }

        __forceinline auto get_exit_code() const noexcept {
            auto result = ui32_t();

            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION);
            if (!handle) _Exit: return result;

            GetExitCodeProcess(handle.get(), LPDWORD(&result));

            goto _Exit;
        }

        __forceinline auto wait(ui32_t milisecounds_timeout = INFINITE) const noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION | SYNCHRONIZE);
            return ui32_t(handle.get() ? WaitForSingleObject(handle.get(), milisecounds_timeout) : null);
        }

        __forceinline auto alive(ui32_t* _status = nullptr) const noexcept {
            if (!_status) {
                auto status = ui32_t();
                _status = &status;
            }

            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION | SYNCHRONIZE);
            if (!handle) return false;

            GetExitCodeProcess(handle.get(), LPDWORD(_status));

            return *_status == STATUS_PENDING && WaitForSingleObject(handle.get(), null) != WAIT_OBJECT_0;
        }

        __forceinline auto suspend() const noexcept {
            return set_suspended(true);
        }

        __forceinline auto resume() const noexcept {
            return set_suspended(false);
        }

        __forceinline auto terminate(long exit_status = EXIT_SUCCESS) const noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            return handle.get() ? NT_SUCCESS(NtTerminateProcess(handle.get(), exit_status)) : false;
        }

        __forceinline auto set_priority(int priority_class) const noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            return bool(handle.get() ? SetPriorityClass(handle.get(), priority_class) : false);
        }

        __forceinline auto get_priority() const noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            return ui32_t(handle.get() ? GetPriorityClass(handle.get()) : null);
        }
        
        __forceinline auto set_privilege(const std::string& name, bool state) noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) return false;

            auto token = handle::native_t();
            auto luid = LUID();

            if (!OpenProcessToken(handle.get(), TOKEN_ADJUST_PRIVILEGES, &token)) return false;

            if (!LookupPrivilegeValueA(nullptr, name.c_str(), &luid)) return false;

            auto privileges = TOKEN_PRIVILEGES();
            privileges.PrivilegeCount = 1;
            privileges.Privileges[0].Luid = luid;
            privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED * state;

            return bool(AdjustTokenPrivileges(token, false, &privileges, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr));
        }

        template<size_t _bufferSize = MAX_PATH + FILENAME_MAX> __forceinline auto get_path() const noexcept {
            auto result = std::string();

            auto handle = temp_handle(_id, _handle, PROCESS_QUERY_INFORMATION);
            if (!handle) _Exit: return result;

            unsigned long buffer_size = _bufferSize;
            char buffer[_bufferSize] = { null };

            if (QueryFullProcessImageNameA(handle.get(), null, buffer, &buffer_size)) {
                result = buffer;
            }

            goto _Exit;
        }

        __forceinline auto get_directory() const noexcept {
            auto path = get_path();

            char drive[MAX_PATH]{ 0 };
            char folder[MAX_PATH]{ 0 };
            _splitpath(path.c_str(), drive, folder, nullptr, nullptr);

            return std::string(drive) + folder;
        }

        __forceinline auto get_name() const noexcept {
            auto path = get_path();

            char name[FILENAME_MAX + 1]{ 0 };
            _splitpath(path.c_str(), nullptr, nullptr, name, nullptr);

            return std::string(name);
        }

        __forceinline auto get_extension() const noexcept {
            auto path = get_path();

            char extension[FILENAME_MAX + 1]{ 0 };
            _splitpath(path.c_str(), nullptr, nullptr, nullptr, extension);

            return std::string(extension);
        }

        __forceinline auto get_environment() const noexcept {
            auto result = PEB();

            auto handle = temp_handle(_id, _handle, __defaultProcessOpenAccess);
            if (!handle) _Exit: return result;

            auto information = PROCESS_BASIC_INFORMATION();
            auto information_length = ULONG(sizeof(information));
            if (NT_ERROR(NtQueryInformationProcess(handle.get(), ProcessBasicInformation, &information, information_length, &information_length))) goto _Exit;

            NtReadVirtualMemory(handle.get(), information.PebBaseAddress, &result, sizeof(result), nullptr);

            goto _Exit;
        }

        __forceinline auto get_command_line() const noexcept {
            auto result = ncore::compatible_string();

            auto handle = temp_handle(_id, _handle, __defaultProcessOpenAccess);
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

            if (NT_SUCCESS(NtReadVirtualMemory(handle.get(), parameters.CommandLine.Buffer, buffer, length, nullptr))) {
                result = buffer;
            }

            delete[] buffer;

            goto _Exit;
        }

        __forceinline auto get_threads(ui32_t open_access = THREAD_ALL_ACCESS) const noexcept {
            auto result = std::vector<id_t>();

            auto snapshot = handle::native_handle_t(
                CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, _id),
                __snapshotHandleCloser,
                true);

            if (!snapshot) _Exit: return result;

            auto entry = THREADENTRY32{ sizeof(THREADENTRY32) };
            if (!Thread32First(snapshot.get(), &entry)) goto _Exit; //todo: replace this enumeration procedure to native

            do {
                if (entry.th32OwnerProcessID == _id) if (auto thread = ncore::thread::get_handle(entry.th32ThreadID, open_access)) {
                    result.push_back(entry.th32ThreadID);
                    __threadHandleCloser(thread);
                }

                entry.dwSize = sizeof(THREADENTRY32);
            } while (Thread32Next(snapshot.get(), &entry));

            goto _Exit;
        }

        __forceinline auto get_base() const noexcept {
            return get_environment().ImageBaseAddress;
        }

        __forceinline auto get_windows() const noexcept {
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

        __forceinline auto get_modules() const noexcept {
            auto result = std::vector<module_t>();

            static auto callback = [](module_t& module, decltype(result)* result, address_t* _return) {
                result->push_back(module);
                return false;
            };

            enumerate_modules<decltype(result)*>(callback, &result);
            return result;
        }

        __forceinline auto search_module(const std::string& name, module_t* _module = nullptr) const noexcept {
            static auto callback = [](module_t& module, const std::string* name, address_t* _return) noexcept {
                return strings::string_to_lower(module.name()) == *name;
                };

            auto lower_name = strings::string_to_lower(name);
            return enumerate_modules<const std::string*>(callback, &lower_name, _module);
        }

        __forceinline auto get_address_base(address_t address, module_t* _module = nullptr) const noexcept {
            static auto callback = [](module_t& module, ui64_t address, address_t* _return) noexcept {
                return address >= module.address<ui64_t>() && address <= (module.address<ui64_t>() + module.size());
                };

            return enumerate_modules<ui64_t>(callback, ui64_t(address), _module);
        }

        __forceinline auto get_exports(const std::string& module_name, module_t* _module = nullptr) const noexcept {
            auto result = std::vector<module_t::export_t>();

            auto module = module_t();
            if (search_module(module_name, &module)) {
                if (_module) {
                    *_module = module;
                }

                result = module.get_exports();
            }

            return result;
        }

        __forceinline auto search_exports(const std::string& name, size_t max_results_count = 1) const noexcept {
            struct searching_info {
                const process* instance;
                std::vector<module_t::export_t> results;
                size_t max_results_count;
                std::string name;
            };
            
            static auto callback = [](module_t& module, searching_info* data, address_t* _return) noexcept {
                auto export_info = module.search_export(data->name);
                if (export_info.module) {
                    if (data->results.size() >= data->max_results_count) return true;

                    data->results.push_back(export_info);
                }

                return false;
            };

            auto info = searching_info{
                this,
                decltype(searching_info::results)(),
                max_results_count ? max_results_count : 1,
                name 
            };

            enumerate_modules<searching_info*>(callback, &info);

            return info.results;
        }

        __forceinline auto get_regions() const noexcept {
            auto results = std::vector<region_t>();
            constexpr const auto callback = [](handle_t& handle, memory_t& info, decltype(results)* _results, address_t*) noexcept {
                auto region = region_t();
                auto status = NtQueryVirtualMemory(handle.get(), info.region(), MEMORY_INFORMATION_CLASS::MemoryRegionInformation, &region, sizeof(region_t::base_t), nullptr);
                if (NT_SUCCESS(status)) {
                    for (auto& exists : *_results) {
                        if (exists.address() != region.address()) continue;

                        exists.memory().push_back(info);
                        return false;
                    }

                    region.memory().push_back(info);
                    _results->push_back(region);
                }

                return false;
            };

            enumerate_memory<decltype(results)*>(callback, &results);

            return results;
        }

        __forceinline auto get_address_region(address_t address, region_t* _region = nullptr) const noexcept {
            if (!address) return address_t(nullptr);
            
            auto regions = get_regions();
            for (auto& region : regions) {
                if (!region.bounds().in_range(address)) continue;

                if (_region) {
                    *_region = region;
                }

                return region.address();
            }

            return address_t(nullptr);
        }

        __forceinline bool is_memory_available(address_t address) const noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) return false;

            auto region_info = MEMORY_BASIC_INFORMATION();

            return NT_SUCCESS(NtQueryVirtualMemory(handle.get(), address, MEMORY_INFORMATION_CLASS::MemoryBasicInformation, &region_info, sizeof(region_info), nullptr)) ?
                !bool(region_info.State & MEM_FREE) :
                false;
        }

        __forceinline ui32_t get_memory_protect(address_t address) const noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) return null;

            auto region_info = MEMORY_BASIC_INFORMATION();

            return NT_SUCCESS(NtQueryVirtualMemory(handle.get(), address, MEMORY_INFORMATION_CLASS::MemoryBasicInformation, &region_info, sizeof(region_info), nullptr)) ?
                ui32_t(region_info.Protect) :
                null;
        }

        __forceinline bool set_memory_protect(address_t address, size_t size, ui32_t protect, ui32_t* _previous = nullptr) noexcept {
            auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
            if (!handle) return false;

            if (!_previous) {
                auto value = ui32_t();
                _previous = &value;
            }

            return NT_SUCCESS(NtProtectVirtualMemory(handle.get(), &address, &size, protect, PULONG(_previous)));
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

            result =
                //NT_SUCCESS(NtFreeVirtualMemory(handle.get(), &address, &size, MEM_DECOMMIT)) &&
                NT_SUCCESS(NtFreeVirtualMemory(handle.get(), &address, &size, MEM_RELEASE));

            goto _Exit;
        }

        __forceinline constexpr bool write_memory(address_t address, const void* data, size_t size) noexcept {
            if (address && data && size) {
                if (_id == __process_id) return memcpy(address, data, size);

                auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
                auto handle_value = handle.get();
                if (handle_value) return NT_SUCCESS(NtWriteVirtualMemory(handle_value, address, address_t(data), size, nullptr));
            }

            return false;
        }

        template<typename _t> __forceinline constexpr bool write_memory(address_t address, const _t& data) noexcept {
            return write_memory(address, &data, sizeof(_t));
        }

        __forceinline constexpr bool read_memory(address_t address, size_t size, void* _data) const noexcept {
            if (address && size && _data) {
                if (_id == __process_id) return memcpy(_data, address, size);
                
                auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS);
                auto handle_value = handle.get();
                if (handle_value) return NT_SUCCESS(NtReadVirtualMemory(handle_value, address, _data, size, nullptr));
            }

            return false;
        }

        template<typename _t> __forceinline constexpr _t read_memory(address_t address) const noexcept {
            auto result = _t();
            read_memory(address, sizeof(_t), &result);
            return result;
        }

        __forceinline constexpr bool dump_memory(address_t address, size_t size, void* _data) const noexcept {
            if (!(size && _data)) return false;

            const auto handle = temp_handle(_id, _handle, PROCESS_ALL_ACCESS).get();
            if (!handle) return false;

            auto remain = size;
            auto readed = size_t(), offset = offset_t();
            do {
                auto block_info = MEMORY_BASIC_INFORMATION();
                if (NT_ERROR(NtQueryVirtualMemory(handle, byte_p(address) + offset, MEMORY_INFORMATION_CLASS::MemoryBasicInformation, &block_info, sizeof(block_info), nullptr))) break;

                if (!block_info.RegionSize) {
                    block_info.RegionSize = PAGE_SIZE;
                }

                if (block_info.State == MEM_FREE || block_info.Protect == PAGE_NOACCESS) {
                    readed = block_info.RegionSize;
                }
                else {
                    auto count = remain;
                    do {
                        auto source = byte_p(address) + offset;
                        auto destination = byte_p(_data) + offset;

                        NtReadVirtualMemory(handle, source, destination, count, &readed);
                        if (readed) break;

                        if (!count) return true;

                        if (source == block_info.BaseAddress) {
                            if (count == block_info.RegionSize) {
                                readed = block_info.RegionSize;

                                break;
                            }

                        }
                        else {
                            offset -= (ui64_t(source) - ui64_t(block_info.BaseAddress));
                        }

                        count = block_info.RegionSize;
                    } while (true);
                }

                if (readed >= remain) break;

                remain -= readed;
                offset += readed;
            } while (remain > 0);

            return true;
        }

        __forceinline auto create_thread(address_t start, void* parameter = nullptr, bool keep_handle = false, int priority = null, unsigned flags = null, size_t stack_size = null) noexcept {
            return thread::create(start, parameter, temp_handle(_id, _handle, PROCESS_ALL_ACCESS).get(), keep_handle, priority, flags, stack_size);
        }

        __forceinline auto load_library(const ncore::path& file, module_t* _module = nullptr) noexcept {
            auto result = address_t();

            if (file.string().empty()) _Exit: return result;

            auto offset = sizeof(ui32_t) * bool(*ui32_p(file.string().c_str()) == *ui32_p("\\??\\"));

            if (_id == __process_id) return address_t(LoadLibraryA(file.string().c_str() + offset));

            auto address = allocate_memory(file.string().length(), PAGE_READWRITE);
            if (!address) goto _Exit;

            if (write_memory(address, file.string().c_str(), file.string().length())) {
                if (auto procedure = load_library_t(search_exports("LoadLibraryA", 1).front().address)) {
                    create_thread(procedure, address_t(ui64_t(address) + offset)).wait();
                    result = search_module(file.name().string(), _module);
                }
            }

            release_memory(address);

            return result;
        }
    };

    template<unsigned _bufferSize> static __forceinline std::vector<process> get_processes(unsigned open_access) {
        std::vector<process> results;

        id_t buffer[_bufferSize] = { null };
        unsigned long count = null;

        if (!K32EnumProcesses(buffer, sizeof(id_t) * _bufferSize, &count)) _Exit: return results;
        count /= sizeof(id_t);

        for (unsigned i = null; i < count; i++) {
            auto handle = ncore::process::get_handle(buffer[i], open_access);
            if (!handle) continue;

            __processHandleCloser(handle);

            results.push_back(process(buffer[i]));
        }

        goto _Exit;
    }
}