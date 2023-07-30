#pragma once
#include "defines.hpp"
#include "string_utils.hpp"
#include "thread.hpp"
#include <windows.h>
#include <tlhelp32.h>

#pragma intrinsic(_ReturnAddress)

#define PAGE_ROUND_UP(X) align_up(unsigned __int64(X), PAGE_SIZE)
#define PAGE_ROUND_DOWN(X) align_down(unsigned __int64(X), PAGE_SIZE)

#define align_up(VALUE, ALIGNMENT) (((VALUE) + (ALIGNMENT) - 1) & ~((ALIGNMENT) - 1))
#define align_down(VALUE, ALIGNMENT) ((VALUE) & ~((ALIGNMENT) - 1))

#define minmax(MIN, VAL, MAX) max(min(VAL, MAX), MIN)
#define normalize(VAL, MIN, MAX) ((VAL - MIN) / (MAX - MIN))
#define absoulte(VAL) VAL > 0 ? VAL : -VAL

#define rand_in_range(MIN, MAX) (MIN + ((long long(GetTickCount64() ^ GetCurrentThreadId())) % (MAX - MIN)))

#define is_lowercase_input(VK_SHIFT_PRESSED) ((bool)(!((((GetKeyState(VK_CAPITAL) & 0x0001) != 0) && !((bool)(VK_SHIFT_PRESSED))) ? (true) : ((!((GetKeyState(VK_CAPITAL) & 0x0001) != 0) && ((bool)(VK_SHIFT_PRESSED))) ? true : false))))

#define messagef(TYPE, TITLE, ...) MessageBoxA(NULL, ncore::string_utils::format(__VA_ARGS__).c_str(), (TITLE), (TYPE))

namespace ncore::utils {
    using namespace std;

    struct reading_thread_info {
        address_t progress;
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
#ifdef _WIN64
        data.pRtlAddFunctionTable = (f_RtlAddFunctionTable)RtlAddFunctionTable;
#else 
        SEHExceptionSupport = false;
#endif
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

                    if (NT_ERROR(NtQueryVirtualMemory(args->process, block_address, MemoryBasicInformation, &block_info, sizeof(block_info), NULL))){
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

        _info->alloc(threads_count);


        auto size_per_thread = (size / threads_count);

        for (size_t i = 0; i < threads_count; i++) {
            auto arguments = new reading_arguments {
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

        auto WindowBorders = RECT {
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

    static __forceinline HKL get_current_keyboard_layout() {
        struct local {
            static __declspec(noinline) void get(HKL* _result) { if (_result) *_result = GetKeyboardLayout(NULL); }
        };

        HKL Result = NULL;
        auto ThreadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)(&local::get), (LPVOID)(&Result), NULL, NULL);
        if (ThreadHandle) {
            WaitForSingleObject(ThreadHandle, INFINITE);
            CloseHandle(ThreadHandle);
        }

        return Result;
    }

    static __forceinline char vk_to_ascii(int vkCode) {
        BYTE KeysStates[256]{ 0 };
        KeysStates[VK_CAPITAL] = GetKeyState(VK_CAPITAL);
        KeysStates[VK_SHIFT] = GetKeyState(VK_SHIFT);

        HKL KeyboardLayout = get_current_keyboard_layout();
        UINT ScanCode = MapVirtualKeyExW(vkCode, MAPVK_VK_TO_VSC, KeyboardLayout);

        WORD Result = NULL;
        ToAsciiEx(vkCode, ScanCode, KeysStates, &Result, NULL, KeyboardLayout);

        return *(char*)&Result;
    }

    static __forceinline wchar_t vk_to_unicode(int vkCode) {
        BYTE KeysStates[256]{ 0 };
        KeysStates[VK_CAPITAL] = GetKeyState(VK_CAPITAL);
        KeysStates[VK_SHIFT] = GetKeyState(VK_SHIFT);

        HKL KeyboardLayout = get_current_keyboard_layout();
        UINT ScanCode = MapVirtualKeyExW(vkCode, MAPVK_VK_TO_VSC, KeyboardLayout);

        WCHAR Result = NULL;
        ToUnicodeEx(vkCode, ScanCode, KeysStates, &Result, 1, NULL, KeyboardLayout);

        return *(wchar_t*)&Result;
    }

    static __forceinline std::string get_error_description(unsigned error_code) {
        LPVOID buffer = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, NULL);

        return buffer ? std::string((char*)buffer) : std::string("unknown");
    }
}