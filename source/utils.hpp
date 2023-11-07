#pragma once
#include "defines.hpp"
#include "strings.hpp"
#include "dimension_vector.hpp"
#include "thread.hpp"
#include "file.hpp"
#include <tlhelp32.h>

#define minmax(MIN, VAL, MAX) max(min(VAL, MAX), MIN)
#define normalize(VAL, MIN, MAX) (((VAL) - (MIN)) / ((MAX) - (MIN)))
#define ABSOLUTE(VAL) (((VAL) > 0) ? (VAL) : (-VAL))

#define rand_in_range(MIN, MAX) (MIN + ((long long(GetTickCount64() ^ __thread_id)) % (MAX - MIN)))

#define messagef(TYPE, TITLE, ...) MessageBoxA(NULL, ncore::format_string(__VA_ARGS__).c_str(), (TITLE), (TYPE))

namespace ncore {
    namespace utils {
        using namespace strings;

        struct reading_thread_info {
            address_t progress;
        };

        struct system_version {
            ui32_t major, minor, build;
        };

        using reading_info = multi_thread_info<reading_thread_info>;

        static __forceinline address_t manual_map_library(handle::native_t process, byte_t* file, size_t size, unsigned fdwReason = DLL_PROCESS_ATTACH, address_t lpReserved = NULL, bool clearHeader = true, bool clearNonNeededSections = true, bool adjustProtections = true, bool sehExceptionSupport = true) {
            using f_LoadLibraryA = HINSTANCE(WINAPI*)(const char* lpLibFilename);

            using f_GetProcAddress = FARPROC(WINAPI*)(HMODULE hModule, LPCSTR lpProcName);

            using f_DLL_ENTRY_POINT = BOOL(WINAPI*)(void* hDll, DWORD dwReason, void* pReserved);

            using f_RtlAddFunctionTable = BOOL(WINAPIV*)(PRUNTIME_FUNCTION FunctionTable, DWORD EntryCount, DWORD64 BaseAddress);


            struct MANUAL_MAPPING_DATA
            {
                f_LoadLibraryA pLoadLibraryA;
                f_GetProcAddress pGetProcAddress;

                f_RtlAddFunctionTable pRtlAddFunctionTable;

                BYTE* pbase;
                HINSTANCE hMod;
                DWORD fdwReasonParam;
                LPVOID reservedParam;
                BOOL SEHSupport;
            };

            struct local {
                static __declspec(noinline) void __stdcall Shellcode(MANUAL_MAPPING_DATA* pData)
                {
                    if (!pData)
                    {
                        pData->hMod = (HINSTANCE)0x404040;
                        return;
                    }

                    BYTE* pBase = pData->pbase;
                    auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)pBase)->e_lfanew)->OptionalHeader;

                    auto _LoadLibraryA = pData->pLoadLibraryA;
                    auto _GetProcAddress = pData->pGetProcAddress;
#ifdef _WIN64
                    auto _RtlAddFunctionTable = pData->pRtlAddFunctionTable;
#endif
                    auto _DllMain = reinterpret_cast<f_DLL_ENTRY_POINT>(pBase + pOpt->AddressOfEntryPoint);

                    BYTE* LocationDelta = pBase - pOpt->ImageBase;
                    if (LocationDelta)
                    {
                        if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
                        {
                            auto* pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
                            const auto* pRelocEnd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(pRelocData) + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
                            while (pRelocData < pRelocEnd && pRelocData->SizeOfBlock)
                            {
                                UINT AmountOfEntries = (pRelocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                                WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);

                                for (UINT i = 0; i != AmountOfEntries; ++i, ++pRelativeInfo)
                                {
                                    if (((*pRelativeInfo) >> 0x0C) == IMAGE_REL_BASED_DIR64)
                                    {
                                        UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + ((*pRelativeInfo) & 0xFFF));
                                        *pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
                                    }

                                }

                                pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
                            }

                        }

                    }

                    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
                    {
                        auto* pImportDescr = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
                        while (pImportDescr->Name)
                        {
                            char* szMod = reinterpret_cast<char*>(pBase + pImportDescr->Name);
                            HINSTANCE hDll = _LoadLibraryA(szMod);

                            ULONG_PTR* pThunkRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->OriginalFirstThunk);
                            ULONG_PTR* pFuncRef = reinterpret_cast<ULONG_PTR*>(pBase + pImportDescr->FirstThunk);

                            if (!pThunkRef)
                                pThunkRef = pFuncRef;

                            for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
                            {
                                if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
                                {
                                    *pFuncRef = (ULONG_PTR)_GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
                                }
                                else
                                {
                                    auto* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + (*pThunkRef));
                                    *pFuncRef = (ULONG_PTR)_GetProcAddress(hDll, pImport->Name);
                                }

                            }

                            ++pImportDescr;
                        }

                    }

                    if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
                    {
                        auto* pTLS = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
                        auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTLS->AddressOfCallBacks);

                        for (; pCallback && *pCallback; ++pCallback)
                            (*pCallback)(pBase, DLL_PROCESS_ATTACH, NULL);
                    }

                    bool ExceptionSupportFailed = false;

#ifdef _WIN64

                    if (pData->SEHSupport)
                    {
                        auto excep = pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
                        if (excep.Size)
                        {
                            if (!_RtlAddFunctionTable(
                                reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY*>(pBase + excep.VirtualAddress),
                                excep.Size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY), (DWORD64)pBase))
                            {
                                ExceptionSupportFailed = true;
                            }

                        }

                    }

#endif

                    _DllMain(pBase, pData->fdwReasonParam, pData->reservedParam);

                    if (ExceptionSupportFailed)
                        pData->hMod = reinterpret_cast<HINSTANCE>(0x505050);
                    else
                        pData->hMod = reinterpret_cast<HINSTANCE>(pBase);
                }
            };


            IMAGE_NT_HEADERS* pOldNtHeader = NULL;
            IMAGE_OPTIONAL_HEADER* pOldOptHeader = NULL;
            IMAGE_FILE_HEADER* pOldFileHeader = NULL;
            BYTE* pTargetBase = NULL;

            if (reinterpret_cast<IMAGE_DOS_HEADER*>(file)->e_magic != 0x5A4D) //"MZ"
            {
                return NULL;
            }

            pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(file + reinterpret_cast<IMAGE_DOS_HEADER*>(file)->e_lfanew);
            pOldOptHeader = &pOldNtHeader->OptionalHeader;
            pOldFileHeader = &pOldNtHeader->FileHeader;

            if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
            {
                return NULL;
            }

            pTargetBase = reinterpret_cast<BYTE*>(VirtualAllocEx(process, NULL, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!pTargetBase)
            {
                return NULL;
            }

            DWORD oldp = 0;
            VirtualProtectEx(process, pTargetBase, pOldOptHeader->SizeOfImage, PAGE_EXECUTE_READWRITE, &oldp);

            MANUAL_MAPPING_DATA data{ 0 };
            data.pLoadLibraryA = LoadLibraryA;
            data.pGetProcAddress = GetProcAddress;
            data.pRtlAddFunctionTable = (f_RtlAddFunctionTable)RtlAddFunctionTable;
            data.pbase = pTargetBase;
            data.fdwReasonParam = fdwReason;
            data.reservedParam = lpReserved;
            data.SEHSupport = sehExceptionSupport;


            //File header
            if (NtWriteVirtualMemory(process, pTargetBase, file, 0x1000, NULL)) //only first 0x1000 bytes for the header
            {
                VirtualFreeEx(process, pTargetBase, 0, MEM_RELEASE);
                return NULL;
            }

            IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
            for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
            {
                if (pSectionHeader->SizeOfRawData)
                {
                    if (NtWriteVirtualMemory(process, (pTargetBase + pSectionHeader->VirtualAddress), file + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, NULL))
                    {
                        VirtualFreeEx(process, pTargetBase, 0, MEM_RELEASE);
                        return NULL;
                    }

                }

            }

            //Mapping params
            BYTE* MappingDataAlloc = reinterpret_cast<BYTE*>(VirtualAllocEx(process, NULL, sizeof(MANUAL_MAPPING_DATA), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!MappingDataAlloc)
            {
                VirtualFreeEx(process, pTargetBase, NULL, MEM_RELEASE);
                return NULL;
            }

            if (NtWriteVirtualMemory(process, MappingDataAlloc, &data, sizeof(MANUAL_MAPPING_DATA), NULL))
            {
                VirtualFreeEx(process, pTargetBase, NULL, MEM_RELEASE);
                VirtualFreeEx(process, MappingDataAlloc, NULL, MEM_RELEASE);
                return NULL;
            }

            //Shell code
            void* pShellcode = VirtualAllocEx(process, NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (!pShellcode)
            {
                VirtualFreeEx(process, pTargetBase, NULL, MEM_RELEASE);
                VirtualFreeEx(process, MappingDataAlloc, NULL, MEM_RELEASE);
                return NULL;
            }

            if (NtWriteVirtualMemory(process, pShellcode, local::Shellcode, 0x1000, NULL))
            {
                VirtualFreeEx(process, pTargetBase, NULL, MEM_RELEASE);
                VirtualFreeEx(process, MappingDataAlloc, NULL, MEM_RELEASE);
                VirtualFreeEx(process, pShellcode, NULL, MEM_RELEASE);
                return NULL;
            }

            handle::native_t hThread = CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)pShellcode, MappingDataAlloc, NULL, NULL);
            if (!hThread)
            {
                VirtualFreeEx(process, pTargetBase, 0, MEM_RELEASE);
                VirtualFreeEx(process, MappingDataAlloc, 0, MEM_RELEASE);
                VirtualFreeEx(process, pShellcode, 0, MEM_RELEASE);
                return 0;
            }
            CloseHandle(hThread);


            HINSTANCE hCheck = NULL;
            while (!hCheck)
            {
                DWORD exitcode = 0;
                GetExitCodeProcess(process, &exitcode);
                if (exitcode != STILL_ACTIVE)
                {
                    return NULL;
                }

                MANUAL_MAPPING_DATA data_checked{ 0 };
                NtReadVirtualMemory(process, MappingDataAlloc, &data_checked, sizeof(data_checked), NULL);
                hCheck = data_checked.hMod;

                if (hCheck == (HINSTANCE)0x404040)
                {
                    VirtualFreeEx(process, pTargetBase, 0, MEM_RELEASE);
                    VirtualFreeEx(process, MappingDataAlloc, 0, MEM_RELEASE);
                    VirtualFreeEx(process, pShellcode, 0, MEM_RELEASE);
                    return NULL;
                }

                Sleep(10);
            }

            BYTE* emptyBuffer = (BYTE*)malloc(1024 * 1024 * 20);
            if (emptyBuffer == NULL)
            {
                return NULL;
            }
            memset(emptyBuffer, BYTE(0), 1024 * 1024 * 20);

            //CLEAR PE HEAD
            if (clearHeader)
            {
                NtWriteVirtualMemory(process, pTargetBase, emptyBuffer, 0x1000, NULL);
            }
            //END CLEAR PE HEAD


            if (clearNonNeededSections)
            {
                pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
                for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
                {
                    if (pSectionHeader->Misc.VirtualSize)
                    {
                        if ((sehExceptionSupport ? 0 : strcmp((char*)pSectionHeader->Name, ".pdata") == 0) || strcmp((char*)pSectionHeader->Name, ".rsrc") == 0 || strcmp((char*)pSectionHeader->Name, ".reloc") == 0)
                        {
                            NtWriteVirtualMemory(process, (pTargetBase + pSectionHeader->VirtualAddress), emptyBuffer, pSectionHeader->Misc.VirtualSize, NULL);
                        }

                    }

                }

            }

            if (adjustProtections)
            {
                pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
                for (UINT i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
                {
                    if (pSectionHeader->Misc.VirtualSize)
                    {
                        DWORD old = 0;
                        DWORD newP = PAGE_READONLY;

                        if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) > 0)
                        {
                            newP = PAGE_READWRITE;
                        }
                        else if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) > 0)
                        {
                            newP = PAGE_EXECUTE_READ;
                        }

                        VirtualProtectEx(process, pTargetBase + pSectionHeader->VirtualAddress, pSectionHeader->Misc.VirtualSize, newP, &old);
                    }

                }

                DWORD old = 0;
                VirtualProtectEx(process, pTargetBase, IMAGE_FIRST_SECTION(pOldNtHeader)->VirtualAddress, PAGE_READONLY, &old);
            }

            NtWriteVirtualMemory(process, pShellcode, emptyBuffer, 0x1000, NULL);

            VirtualFreeEx(process, pShellcode, NULL, MEM_RELEASE);

            VirtualFreeEx(process, MappingDataAlloc, NULL, MEM_RELEASE);

            return pTargetBase;
        }

        static __forceinline bool can_access(address_t address)
        {
            __try {
                auto byte = *(byte_t*)address;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }

            //or queryvirtualmemory and check

            return true;
        }

        static __forceinline byte_t* dump_process_memory(handle::native_t process, address_t address, size_t size, byte_t* buffer = nullptr, int threads_count = 1, int threads_priority = THREAD_PRIORITY_HIGHEST, reading_info* _info = nullptr) {
            struct reading_arguments {
                reading_info::specific_thread_info* info;

                handle::native_t process;
                address_t start_address;

                offset_t start_offset;
                size_t reading_size;

                byte_t* buffer;
            };

            struct local {
                static __declspec(noinline) void read(reading_arguments* args) {
                    for (offset_t offset = 0; offset < args->reading_size;) {
                        auto bytes_readed = size_t(0);

                        auto block_address = (args->info->progress = address_t(ui64_t(args->start_address) + args->start_offset + offset));
                        auto block_info = MEMORY_BASIC_INFORMATION{ 0 };

                        if (NT_ERROR(NtQueryVirtualMemory(args->process, block_address, MemoryBasicInformation, &block_info, sizeof(block_info), NULL))) {
                            bytes_readed = 1;
                            goto _Continue;
                        }

                        if (block_info.State == MEM_FREE || block_info.Protect == PAGE_NOACCESS) {
                        _InvalidRegion:
                            if (!block_info.RegionSize) break;

                            bytes_readed = block_info.RegionSize;

                        _Continue:
                            offset += bytes_readed;

                            continue;
                        }

                        auto reading_size = min(args->reading_size, block_info.RegionSize - (ui64_t(block_address) - ui64_t(block_info.BaseAddress)));
                        if (NT_ERROR(NtReadVirtualMemory(args->process, block_address, (args->buffer + args->start_offset + offset), reading_size, &bytes_readed)) || !bytes_readed) {
                        _NoMemoryReaded:
                            memset((args->buffer + args->start_offset + offset), 0, reading_size);

                            goto _InvalidRegion;
                        }

                        goto _Continue;
                    }

                    return delete args;
                }
            };

            if (!(process && size && threads_count)) _Fail: return nullptr;

            if (!buffer) {
                buffer = (byte_t*)malloc(size);
            }

            if (!_info) {
                auto info_buffer = reading_info(true);

                _info = &info_buffer;
            }

            _info->alloc(threads_count);


            auto size_per_thread = (size / threads_count);

            for (size_t i = 0; i < threads_count; i++) {
                auto arguments = new reading_arguments{
                    &(_info->threads[i]),

                    process,
                    address,

                    size_per_thread * i,
                    size_per_thread,

                    buffer
                };

                _info->threads[i].thread = ncore::thread::create(local::read, arguments, nullptr, threads_priority);
            }

            do {
                auto alive_threads_count = size_t(0);
                for (size_t i = 0; i < threads_count; i++)
                    alive_threads_count += _info->threads[i].thread.alive();

                if (!(_info->alive_threads_count = alive_threads_count)) break;

                SleepEx(250, FALSE);
            } while (true);

            if (_info->release_after_using) {
                _info->release();
            }

            return buffer;
        }

        static __forceinline bool get_window_rectangles(HWND window, POINT* _pos, SIZE* _size, RECT* _borders) {
            if (!window) return false;

            auto window_rect = RECT{ 0 };
            if (!GetWindowRect(window, &window_rect)) _Fail: return false;

            auto client_rect = RECT{ 0 };
            if (!GetClientRect(window, &client_rect)) goto _Fail;

            auto result = true;

            if (_size) {
                _size->cx = client_rect.right;
                _size->cy = client_rect.bottom;
                result &= _size->cx > 0 && _size->cy > 0;
            }

            MapWindowPoints(window, null, (LPPOINT)&client_rect, 2);

            auto WindowBorders = RECT{
                client_rect.left - window_rect.left,
                client_rect.top - window_rect.top,
                window_rect.right - client_rect.right,
                window_rect.bottom - client_rect.bottom
            };

            if (_borders) {
                *_borders = WindowBorders;
            }

            if (_pos) {
                _pos->x = window_rect.left + WindowBorders.left;
                _pos->y = window_rect.top + WindowBorders.top;
            }

            return result;
        }

        static __forceinline unsigned __int64 system_boot_time() {
            static auto time = unsigned __int64(0);
            if (!time) {
                struct {
                    unsigned __int64 boot;
                    unsigned __int64 current;
                    unsigned __int64 time_zone_bias;
                    unsigned __int32 time_zone_id;
                }system_time;

                if (NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation, &system_time, sizeof(system_time), 0))) {
                    time = system_time.boot;
                }
            }

            return time;
        }

        static __forceinline MODULEENTRY32* get_adress_module(address_t address) {
            MODULEENTRY32* Result = NULL;
            MODULEENTRY32 CurrentModule{ (DWORD32)sizeof(MODULEENTRY32) };

            auto Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
            if (Snapshot == INVALID_HANDLE_VALUE || !Snapshot) {
                return Result;
            }

            if (Module32First(Snapshot, &CurrentModule)) {
                do {
                    if (!((DWORD64)address > (DWORD64)CurrentModule.modBaseAddr && (DWORD64)address < (DWORD64)CurrentModule.modBaseAddr + CurrentModule.modBaseSize))
                        continue;

                    Result = new MODULEENTRY32(CurrentModule);
                    break;
                } while (Module32Next(Snapshot, &CurrentModule));

            }

            CloseHandle(Snapshot);

            return Result;
        }

        static __forceinline HWND get_console_window(bool create = false) {
            auto result = GetConsoleWindow();
            if (result || !create) _Exit: return result;

            if (!AllocConsole()) return nullptr;

            // std::cout, std::clog, std::cerr, std::cin
            FILE* fDummy;
            freopen_s(&fDummy, "CONOUT$", "w", stdout);
            freopen_s(&fDummy, "CONOUT$", "w", stderr);
            freopen_s(&fDummy, "CONIN$", "r", stdin);
            std::cout.clear();
            std::clog.clear();
            std::cerr.clear();
            std::cin.clear();

            // std::wcout, std::wclog, std::wcerr, std::wcin
            auto hConOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            auto hConIn = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
            SetStdHandle(STD_ERROR_HANDLE, hConOut);
            SetStdHandle(STD_INPUT_HANDLE, hConIn);
            std::wcout.clear();
            std::wclog.clear();
            std::wcerr.clear();
            std::wcin.clear();

            CloseHandle(hConOut);
            CloseHandle(hConIn);

            result = GetConsoleWindow();

            goto _Exit;
        }

        static __forceinline std::string get_error_description(unsigned error_code) {
            char* buffer = nullptr;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error_code, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (char*)&buffer, 0, NULL);

            return buffer ? std::string(buffer) : std::string("unknown");
        }

        static __forceinline vec2f get_screen_scale() {
            auto monitor = MonitorFromWindow(NULL, MONITOR_DEFAULTTONEAREST);

            auto monitor_info = MONITORINFOEXA{ NULL };
            monitor_info.cbSize = sizeof(monitor_info);
            GetMonitorInfoA(monitor, &monitor_info);

            auto dev_mode = DEVMODEA{ NULL };
            dev_mode.dmSize = sizeof(dev_mode);
            dev_mode.dmDriverExtra = 0;
            EnumDisplaySettingsA(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &dev_mode);

            return {
                float(dev_mode.dmPelsWidth) / float(monitor_info.rcMonitor.right - monitor_info.rcMonitor.left),
                float(dev_mode.dmPelsHeight) / float(monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top) };
        }

        static __forceinline vec2i32 get_scaled_window_size(HWND window, bool client_part) {
            if (!window) return vec2i32();

            auto rectangle = RECT();
            auto get_rectangle = client_part ? GetClientRect : GetWindowRect;
            get_rectangle(window, &rectangle);

            auto scale = get_screen_scale();

            return vec2i32(
                (float(rectangle.right - rectangle.left) * scale.x),
                (float(rectangle.bottom - rectangle.top) * scale.y));
        }

        static __forceinline vec2i32 get_scaled_screen_size() {
            return get_scaled_window_size(GetDesktopWindow(), null);
        }

        static __forceinline size_t get_bitmap_buffer_size(vec2i32 size) {
            return (((24 * size.x + 31) & ~31) / 8) * size.y;
        }

        static __forceinline size_t get_bitmap_data_size(size_t bitmap_buffer_size) {
            return sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bitmap_buffer_size;
        }

        static __forceinline size_t get_bitmap_data_size(vec2i32 size) {
            return get_bitmap_data_size(get_bitmap_buffer_size(size));
        }

        static __forceinline bool save_dc_to_bitmap(HDC dc, vec2i32 size, byte_t* _buffer, size_t* _buffer_size) {
            if (!(dc && size.x && size.y && _buffer_size)) return false;

            auto bitmap_buffer_size = get_bitmap_buffer_size(size);
            auto buffer_size = get_bitmap_data_size(bitmap_buffer_size);
            if (*_buffer_size < buffer_size) {
                *_buffer_size = buffer_size;
                return false;
            }

            if (!_buffer) return false;

            auto bitmap_file_header = BITMAPFILEHEADER();
            bitmap_file_header.bfType = (WORD)('B' | ('M' << 8));
            bitmap_file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

            auto bitmap_info = BITMAPINFO();
            auto& bitmap_info_header = bitmap_info.bmiHeader;
            bitmap_info_header.biSize = sizeof(BITMAPINFOHEADER);
            bitmap_info_header.biBitCount = 24;
            bitmap_info_header.biCompression = BI_RGB;
            bitmap_info_header.biPlanes = 1;
            bitmap_info_header.biWidth = size.x;
            bitmap_info_header.biHeight = size.y;

            auto bitmap_buffer = address_t(nullptr);

            auto memory_dc = CreateCompatibleDC(dc);
            auto bitmap = CreateDIBSection(dc, &bitmap_info, DIB_RGB_COLORS, (void**)&bitmap_buffer, NULL, 0);
            SelectObject(memory_dc, bitmap);
            BitBlt(memory_dc, 0, 0, size.x, size.y, dc, 0, 0, SRCCOPY);

            auto result = _buffer;

            memcpy(result, &bitmap_file_header, sizeof(BITMAPFILEHEADER));
            memcpy(result + sizeof(BITMAPFILEHEADER), &bitmap_info_header, sizeof(BITMAPINFOHEADER));
            memcpy(result + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), bitmap_buffer, bitmap_buffer_size);

            DeleteDC(memory_dc);
            DeleteObject(bitmap);

            return result;
        }

        static __forceinline byte_t* save_dc_to_bitmap(HDC dc, vec2i32 size, size_t* _size) {
            auto& buffer_size = *_size = size_t(0);
            save_dc_to_bitmap(dc, size, nullptr, &buffer_size);

            auto buffer = (byte_t*)malloc(buffer_size);
            if (!save_dc_to_bitmap(dc, size, buffer, &buffer_size)) {
                free(buffer);
                return nullptr;
            }

            return buffer;
        }

        static __forceinline bool capture_window(HWND window, bool client_part, byte_t* _buffer, size_t* _size) {
            if (!window) return false;

            auto dc = GetDC(window);
            auto result = save_dc_to_bitmap(dc, get_scaled_window_size(window, client_part), _buffer, _size);
            DeleteDC(dc);

            return result;
        }

        static __forceinline byte_t* capture_window(HWND window, bool client_part, size_t* _size) {
            if (!window) return nullptr;

            auto dc = GetDC(window);
            auto result = save_dc_to_bitmap(dc, get_scaled_window_size(window, client_part), _size);
            DeleteDC(dc);

            return result;
        }

        static __forceinline bool capture_screen(byte_t* _buffer, size_t* _size) {
            return capture_window(GetDesktopWindow(), null, _buffer, _size);
        }

        static __forceinline byte_t* capture_screen(size_t* _size) {
            return capture_window(GetDesktopWindow(), null, _size);
        }

        static __forceinline RTL_OSVERSIONINFOW get_system_version_info() {
            static RTL_OSVERSIONINFOW* result = nullptr;
            if (!result) {
                RtlGetVersion(result = new RTL_OSVERSIONINFOW());
            }

            return *result;
        }

        static __forceinline system_version get_system_version() {
            static system_version* result = nullptr;
            if (!result) {
                auto info = get_system_version_info();

                result = new system_version{
                    info.dwBuildNumber >= 22000 ? 11 : info.dwMajorVersion,
                    info.dwMinorVersion,
                    info.dwBuildNumber
                };

            }

            return *result;
        }

        static __forceinline std::vector<std::string> parse_command_line(int argc, char** argv) noexcept {
            auto list = std::vector<std::string>();

            for (int i = 0; i < argc; i++) {
                list.push_back(argv[i]);
            }

            return list;
        }

        static __forceinline count_t erase_launch_references(const std::string& process) noexcept {
            auto count = count_t(0);

            auto target = strings::string_to_lower(process);

            auto directory = std::filesystem::recursive_directory_iterator(system_directory() + "..\\Prefetch");
            for (const auto& file : directory) {
                auto path = strings::string_to_lower(file.path().string());
                if (path.find(target) == std::string::npos) continue;

                count += std::filesystem::remove(path);
            }

            return count;
        }

        static __forceinline void* duplicate(const void* source, size_t length) noexcept {
            return memcpy(malloc(length), source, length);
        }

        static __forceinline void reverse_copy(void* destination, const void* source, size_t length) noexcept {
            auto dst = (byte_t*)destination;
            auto src = (byte_t*)source + length - 1;

            for (size_t i = 0; i < length; i++, dst++, src--) {
                *dst = *src;
            }
        }

        static __forceinline void reverse(void* data, size_t length) noexcept {
            auto buffer = duplicate(data, length);
            reverse_copy(data, buffer, length);
            free(buffer);
        }

        __forceinline auto read_console_line(const std::string& reason = std::string()) noexcept {
            if (!reason.empty())
                printf(reason.c_str());

            auto result = std::string();
            std::getline(std::cin, result);

            return result;
        }
    }

    using namespace utils;
}
