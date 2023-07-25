#pragma once
#include "utils.hpp"
#include "defines.hpp"
#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include "includes/ntos.h"

#pragma warning(disable: 4055)
#pragma warning(error: 4244)
#pragma warning(error: 4267)

#define get_header_dictionary(MODULE, INDEX)  &(MODULE)->headers->OptionalHeader.DataDirectory[INDEX]
#define pointer_offset(DATA, OFFSET) (void*)(((uintptr_t)(DATA)) + (OFFSET))

namespace ncore {
    static constexpr const unsigned const __protectionFlags[2][2][2] = { { {PAGE_NOACCESS, PAGE_WRITECOPY}, {PAGE_READONLY, PAGE_READWRITE}, }, { {PAGE_EXECUTE, PAGE_EXECUTE_WRITECOPY}, {PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE}, }, };

    class silent_library {
    private:
        using dll_entry_t = int(*)(HINSTANCE, DWORD32, LPVOID);
        using exe_entry_t = int (*)(void);
        using hmodule_t = HMODULE;
        using image_headers = IMAGE_NT_HEADERS64;

        struct pointer_list {
            pointer_list* next;
            address_t address;
        };

        struct section_finalize_data {
            address_t address, aligned_address;
            size_t size;
            unsigned characteristics;
            bool last;
        };

        struct export_name_entry {
            short index;
            char* name;
        };

        hmodule_t code_base;
        size_t modules_count;
        hmodule_t* modules;

        bool initialized;
        bool dynamic_link;
        bool relocated;

        size_t page_size;
        exe_entry_t exe_entry;
        dll_entry_t dll_entry;

        image_headers* headers;
        pointer_list* blocked_memory;
        export_name_entry* exports_table;

        __forceinline silent_library() { return; }
        __forceinline ~silent_library() { return; }

        static __declspec(noinline) int compare(const export_name_entry* first, const export_name_entry* second) {
            return strcmp(first->name, second->name);
        }

        static __declspec(noinline) int find(const char** name, const export_name_entry* entry) {
            return strcmp(*name, entry->name);
        }

        static __forceinline void release_pointer_list(pointer_list* node) {
            while (node) {
                VirtualFree(node->address, 0, MEM_RELEASE);
                auto next = node->next;
                free(node);
                node = next;
            }
        }

        static __forceinline bool copy_sections(silent_library* module, const byte_t* data, size_t size, IMAGE_NT_HEADERS* old_headers) {
            byte_t* dest = nullptr;

            auto code_base = (byte_t*)module->code_base;
            auto section = IMAGE_FIRST_SECTION(module->headers);
            for (int i = 0; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
                if (section->SizeOfRawData == 0) {
                    auto section_size = old_headers->OptionalHeader.SectionAlignment;
                    if (section_size > 0) {
                        if (!(dest = (byte_t*)VirtualAlloc(code_base + section->VirtualAddress, section_size, MEM_COMMIT, PAGE_READWRITE))) goto _Fail;

                        dest = code_base + section->VirtualAddress;
                        section->Misc.PhysicalAddress = (DWORD)((uintptr_t)dest & 0xffffffff);
                        memset(dest, 0, section_size);
                    }

                    continue;
                }

                if (size < (section->PointerToRawData + section->SizeOfRawData)) _Fail: return false;

                if (!(dest = (byte_t*)VirtualAlloc(code_base + section->VirtualAddress, section->SizeOfRawData, MEM_COMMIT, PAGE_READWRITE))) goto _Fail;

                dest = code_base + section->VirtualAddress;
                memcpy(dest, data + section->PointerToRawData, section->SizeOfRawData);
                section->Misc.PhysicalAddress = (DWORD)((uintptr_t)dest & 0xffffffff);
            }

            return true;
        }

        static __forceinline bool perform_base_relocation(silent_library* module, ptrdiff_t delta)
        {
            auto code_base = (byte_t*)module->code_base;

            auto directory = get_header_dictionary(module, IMAGE_DIRECTORY_ENTRY_BASERELOC);
            if (!directory->Size) return delta == 0;

            auto relocation = (PIMAGE_BASE_RELOCATION)(code_base + directory->VirtualAddress);
            for (; relocation->VirtualAddress > 0; ) {
                auto dest = code_base + relocation->VirtualAddress;
                auto relInfo = (unsigned short*)pointer_offset(relocation, sizeof(IMAGE_BASE_RELOCATION));
                for (unsigned i = 0; i < ((relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2); i++, relInfo++) {
                    int offset = *relInfo & 0xfff;

                    switch (*relInfo >> 12)
                    {
                    case IMAGE_REL_BASED_ABSOLUTE: default: break;

                    case IMAGE_REL_BASED_HIGHLOW:
                        *((unsigned long*)(dest + offset)) += (unsigned long)delta;
                        break;

                    case IMAGE_REL_BASED_DIR64:
                        *((unsigned long long*)(dest + offset)) += (unsigned long long)delta;
                        break;
                    }
                }

                relocation = (PIMAGE_BASE_RELOCATION)pointer_offset(relocation, relocation->SizeOfBlock);
            }

            return true;
        }

        static __forceinline bool build_import_table(silent_library* module)
        {
            auto code_base = (byte_t*)module->code_base;

            auto directory = get_header_dictionary(module, IMAGE_DIRECTORY_ENTRY_IMPORT);
            if (!directory->Size == 0) _Fail: return true;

            auto import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(code_base + directory->VirtualAddress);
            for (; !IsBadReadPtr(import_descriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR)) && import_descriptor->Name; import_descriptor++) {
                auto library_handle = LoadLibraryA((char*)code_base + import_descriptor->Name);
                if (!library_handle) goto _Fail;

                auto temp = (hmodule_t*)realloc(module->modules, (module->modules_count + 1) * (sizeof(hmodule_t)));
                if (!temp) {
                _FreeLibraryAndExit:
                    FreeLibrary(library_handle);
                    goto _Fail;
                }
                module->modules = temp;
                module->modules[module->modules_count++] = library_handle;

                uintptr_t* thunk_reference;
                FARPROC* func_reference;

                if (import_descriptor->OriginalFirstThunk) {
                    thunk_reference = (uintptr_t*)(code_base + import_descriptor->OriginalFirstThunk);
                    func_reference = (FARPROC*)(code_base + import_descriptor->FirstThunk);
                }
                else {
                    thunk_reference = (uintptr_t*)(code_base + import_descriptor->FirstThunk);
                    func_reference = (FARPROC*)(code_base + import_descriptor->FirstThunk);
                }

                auto status = true;
                for (; *thunk_reference; thunk_reference++, func_reference++) {
                    if (IMAGE_SNAP_BY_ORDINAL(*thunk_reference)) {
                        *func_reference = GetProcAddress(library_handle, (LPCSTR)IMAGE_ORDINAL(*thunk_reference));
                    }
                    else {
                        PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(code_base + (*thunk_reference));
                        *func_reference = GetProcAddress(library_handle, (LPCSTR)&thunkData->Name);
                    }

                    if (!(status = *func_reference)) break;
                }

                if (!status) goto _FreeLibraryAndExit;
            }

            return true;
        }

        static __forceinline size_t get_real_section_size(silent_library* module, IMAGE_SECTION_HEADER* section) {
            auto size = section->SizeOfRawData;
            if (!size) {
                if (section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
                    size = module->headers->OptionalHeader.SizeOfInitializedData;
                }
                else if (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
                    size = module->headers->OptionalHeader.SizeOfUninitializedData;
                }
            }
            return size;
        }

        static __forceinline bool finalize_section(silent_library* module, section_finalize_data* section_data) {

            if (!section_data->size) _Exit: return true;

            if (section_data->characteristics & IMAGE_SCN_MEM_DISCARDABLE) {
                if (section_data->address == section_data->aligned_address && (section_data->last || module->headers->OptionalHeader.SectionAlignment == module->page_size || (section_data->size % module->page_size) == 0)) {
                    VirtualFree(section_data->address, section_data->size, MEM_DECOMMIT);
                }
                goto _Exit;
            }

            auto executable = (section_data->characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            auto readable = (section_data->characteristics & IMAGE_SCN_MEM_READ) != 0;
            auto writeable = (section_data->characteristics & IMAGE_SCN_MEM_WRITE) != 0;

            auto protect = __protectionFlags[executable][readable][writeable];
            auto old_protect = protect;

            if (section_data->characteristics & IMAGE_SCN_MEM_NOT_CACHED) {
                protect |= PAGE_NOCACHE;
            }

            if (!VirtualProtect(section_data->address, section_data->size, protect, (PDWORD)&old_protect))  return false;

            goto _Exit;
        }

        static __forceinline bool finalize_sections(silent_library* module)
        {
            auto section = IMAGE_FIRST_SECTION(module->headers);
            auto image_offset = ((uintptr_t)module->headers->OptionalHeader.ImageBase & 0xffffffff00000000);

            section_finalize_data section_data;
            section_data.address = (address_t)((uintptr_t)section->Misc.PhysicalAddress | image_offset);
            section_data.aligned_address = (address_t)align_down((uintptr_t)section_data.address, module->page_size);
            section_data.size = get_real_section_size(module, section);
            section_data.characteristics = section->Characteristics;
            section_data.last = FALSE;
            section++;

            for (int i = 1; i < module->headers->FileHeader.NumberOfSections; i++, section++) {
                auto section_address = (address_t)((uintptr_t)section->Misc.PhysicalAddress | image_offset);
                auto aligned_address = (address_t)align_down((uintptr_t)section_address, module->page_size);
                auto section_size = get_real_section_size(module, section);

                if (section_data.aligned_address == aligned_address || (uintptr_t)section_data.address + section_data.size > (uintptr_t) aligned_address) {
                    if ((section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 || (section_data.characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0) {
                        section_data.characteristics = (section_data.characteristics | section->Characteristics) & ~IMAGE_SCN_MEM_DISCARDABLE;
                    }
                    else {
                        section_data.characteristics |= section->Characteristics;
                    }
                    section_data.size = (((uintptr_t)section_address) + ((uintptr_t)section_size)) - (uintptr_t)section_data.address;
                    continue;
                }

                if (!finalize_section(module, &section_data)) goto _Fail;
                section_data.address = section_address;
                section_data.aligned_address = aligned_address;
                section_data.size = section_size;
                section_data.characteristics = section->Characteristics;
            }

            section_data.last = TRUE;
            if (!finalize_section(module, &section_data)) _Fail: return false;

            return true;
        }

        static __forceinline void execute_tls(silent_library* module)
        {
            auto code_base = (byte_t*)module->code_base;

            auto directory = get_header_dictionary(module, IMAGE_DIRECTORY_ENTRY_TLS);
            if (!directory->VirtualAddress) return;

            auto tls = (PIMAGE_TLS_DIRECTORY)(code_base + directory->VirtualAddress);
            auto callback = (PIMAGE_TLS_CALLBACK*)tls->AddressOfCallBacks;
            if (!callback) return;

            while (*callback) {
                (*callback)((hmodule_t)code_base, DLL_PROCESS_ATTACH, NULL);
                callback++;
            }
        }

    public:
        static __forceinline silent_library* load(const byte_t* data, size_t size) {
            if (size < sizeof(IMAGE_DOS_HEADER)) _Fail: return nullptr;

            auto dos_header = (PIMAGE_DOS_HEADER)data;
            if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) goto _Fail;

            if (size < (dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS))) goto _Fail;

            auto old_header = (PIMAGE_NT_HEADERS) & ((const unsigned char*)(data))[dos_header->e_lfanew];
            if (!(old_header->Signature == IMAGE_NT_SIGNATURE && old_header->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) || (old_header->OptionalHeader.SectionAlignment & 1)) goto _Fail;

            auto last_section_end = size_t(0);
            auto section = IMAGE_FIRST_SECTION(old_header);
            auto optional_section_size = old_header->OptionalHeader.SectionAlignment;
            for (unsigned i = 0; i < old_header->FileHeader.NumberOfSections; i++, section++) {
                auto section_end = section->VirtualAddress + (section->SizeOfRawData ? section->SizeOfRawData : optional_section_size);
                if (section_end > last_section_end) {
                    last_section_end = section_end;
                }
            }

            SYSTEM_INFO system_info;
            GetNativeSystemInfo(&system_info);

            auto aligned_image_size = align_up(old_header->OptionalHeader.SizeOfImage, system_info.dwPageSize);
            if (aligned_image_size != align_up(last_section_end, system_info.dwPageSize)) goto _Fail;

            auto code = (byte_t*)VirtualAlloc((void*)old_header->OptionalHeader.ImageBase, aligned_image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (!code) {
                if (!(code = (byte_t*)VirtualAlloc(nullptr, aligned_image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))) goto _Fail;
            }

            pointer_list* blocked_memory = nullptr;
            auto node = blocked_memory;
            while ((((uintptr_t)code) >> 32) < (((uintptr_t)(code + aligned_image_size)) >> 32)) {
                if (!(node = (pointer_list*)malloc(sizeof(pointer_list)))) {
                    VirtualFree(code, 0, MEM_RELEASE);

                _FreePointerListAndExit:
                    release_pointer_list(blocked_memory);
                    goto _Fail;
                }

                node->next = blocked_memory;
                node->address = code;
                blocked_memory = node;

                if (!(code = (byte_t*)VirtualAlloc(nullptr, aligned_image_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))) goto _FreePointerListAndExit;
            }

            auto result = (silent_library*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(silent_library));
            if (!result) {
                VirtualFree(code, 0, MEM_RELEASE);
                goto _FreePointerListAndExit;
            }

            result->code_base = (HINSTANCE)code;
            result->dynamic_link = (old_header->FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
            result->page_size = system_info.dwPageSize;
            result->blocked_memory = blocked_memory;

            if (size < old_header->OptionalHeader.SizeOfHeaders) {
            _ReleaseLibraryAndExit:
                result->release();
                goto _Fail;
            }

            auto headers = (byte_t*)VirtualAlloc(code, old_header->OptionalHeader.SizeOfHeaders, MEM_COMMIT, PAGE_READWRITE);

            memcpy(headers, dos_header, old_header->OptionalHeader.SizeOfHeaders);
            result->headers = (PIMAGE_NT_HEADERS) & ((const unsigned char*)(headers))[dos_header->e_lfanew];

            result->headers->OptionalHeader.ImageBase = (uintptr_t)code;

            if (!copy_sections(result, data, size, old_header)) goto _ReleaseLibraryAndExit;

            auto location_delta = (ptrdiff_t)(result->headers->OptionalHeader.ImageBase - old_header->OptionalHeader.ImageBase);
            result->relocated = (location_delta) ? perform_base_relocation(result, location_delta) : true;

            if (!build_import_table(result)) goto _ReleaseLibraryAndExit;

            if (!finalize_sections(result)) goto _ReleaseLibraryAndExit;

            execute_tls(result);

            if (result->headers->OptionalHeader.AddressOfEntryPoint != 0) {
                if (result->dynamic_link) {
                    auto entry = (dll_entry_t)(code + result->headers->OptionalHeader.AddressOfEntryPoint);

                    auto successfull = (*entry)((HINSTANCE)code, DLL_PROCESS_ATTACH, 0);
                    if (!successfull) goto _ReleaseLibraryAndExit;

                    result->initialized = true;
                }
                else {
                    result->exe_entry = (exe_entry_t)(code + result->headers->OptionalHeader.AddressOfEntryPoint);
                }
            }
            else {
                result->exe_entry = nullptr;
            }

            return result;
        }

        __forceinline void release() {
            auto module = this;

            if (module->initialized) {
                auto entry = (dll_entry_t)(module->code_base + module->headers->OptionalHeader.AddressOfEntryPoint);
                (*entry)(module->code_base, DLL_PROCESS_DETACH, 0);
            }

            free(module->exports_table);
            if (module->modules) {
                for (int i = 0; i < module->modules_count; i++) {
                    if (module->modules[i]) {
                        FreeLibrary(module->modules[i]);
                    }
                }

                free(module->modules);
            }

            if (module->code_base) {
                VirtualFree(module->code_base, 0, MEM_RELEASE);
            }

            release_pointer_list(module->blocked_memory);

            HeapFree(GetProcessHeap(), 0, module);
        }

        __forceinline address_t search_export(const char* name) {
            auto module = this;
            auto code_base = (byte_t)module->code_base;

            auto directory = get_header_dictionary(module, IMAGE_DIRECTORY_ENTRY_EXPORT);
            if (!directory->Size) _Fail: return nullptr;

            auto exports = (PIMAGE_EXPORT_DIRECTORY)(code_base + directory->VirtualAddress);
            if (exports->NumberOfNames == 0 || exports->NumberOfFunctions == 0) goto _Fail;

            auto index = 0ui32;
            if (HIWORD(name) == 0) {
                if (LOWORD(name) < exports->Base) goto _Fail;

                index = LOWORD(name) - exports->Base;
            }
            else if (!exports->NumberOfNames) {
                goto _Fail;
            }
            else {
                if (!module->exports_table) {
                    auto nameRef = (unsigned*)(code_base + exports->AddressOfNames);
                    auto ordinal = (unsigned short*)(code_base + exports->AddressOfNameOrdinals);

                    auto entry = (export_name_entry*)malloc(exports->NumberOfNames * sizeof(export_name_entry));
                    if (!(module->exports_table = entry)) goto _Fail;

                    for (int i = 0; i < exports->NumberOfNames; i++, nameRef++, ordinal++, entry++) {
                        entry->name = (char*)(code_base + (*nameRef));
                        entry->index = *ordinal;
                    }

                    qsort(module->exports_table, exports->NumberOfNames, sizeof(export_name_entry), (_CoreCrtNonSecureSearchSortCompareFunction)compare);
                }

                auto found = (const export_name_entry*)bsearch(&name, module->exports_table, exports->NumberOfNames, sizeof(export_name_entry), (_CoreCrtNonSecureSearchSortCompareFunction)find);
                if (!found) goto _Fail;
                index = found->index;
            }

            if (index > exports->NumberOfFunctions) goto _Fail;

            return (address_t)(code_base + (*(DWORD*)(code_base + exports->AddressOfFunctions + (index * 4))));
        }

        __forceinline int call_entry(unsigned call_reason, address_t reserved = nullptr) {
            auto module = this;
            if (!module->relocated) _Fail: return -1;

            if (module->dynamic_link) {
                if (!module->dll_entry) goto _Fail;

                return module->dll_entry(module->code_base, call_reason, reserved);
            }

            if (!module->exe_entry) goto _Fail;

            return module->exe_entry();
        }
    };
};
