#pragma once
#include "action.hpp"
#include "handle.hpp"
#include "defines.hpp"
#include "environment.hpp"
#include "static_array.hpp"

#include <memory>
#include <string>
#include <tuple>

/* available defines:
    NCORE_THREAD_EVENT_CREATION_FAILURE
*/

namespace ncore {
    static constexpr const auto const __defaultThreadOpenAccess = ui32_t(THREAD_ALL_ACCESS);
    static const auto const __threadHandleCloser = handle::native_handle_t::closer_t(NtClose);

    class thread {
    public:
        using context_t = CONTEXT;
        using environment_t = TEB;

        static __forceinline constexpr handle::native_t get_handle(id_t id, ui32_t access = __defaultThreadOpenAccess) {
            auto result = handle::native_t();
            auto attributes = OBJECT_ATTRIBUTES();
            auto client_id = CLIENT_ID();

            client_id.UniqueThread = handle::native_t(id);
            client_id.UniqueProcess = handle::native_t(null);
            InitializeObjectAttributes(&attributes, null, null, null, null);

            return NT_SUCCESS(NtOpenThread(&result, access, &attributes, &client_id)) ? result : nullptr;
        }

        static __forceinline auto get_info(handle::native_t handle) noexcept {
            using info_t = THREAD_BASIC_INFORMATION;

            auto information = info_t();
            if (handle) {
                NtQueryInformationThread(handle, THREADINFOCLASS::ThreadBasicInformation, &information, sizeof(information), nullptr);
            }

            return information;
        }

        static __forceinline auto get_id(handle::native_t handle) noexcept {
            return id_t(get_info(handle).ClientId.UniqueThread);
        }

        static __forceinline auto get_process(handle::native_t handle) noexcept {
            return id_t(get_info(handle).ClientId.UniqueProcess);
        }

        template<bool _base = true> static __forceinline auto get_priority(handle::native_t handle) noexcept {
            if constexpr (_base) return ui32_t(get_info(handle).BasePriority);
            else return ui32_t(get_info(handle).Priority);
        }

        static __forceinline auto set_priority(handle::native_t handle, ui32_t priority) noexcept {
            if (!((priority & 0x30000) == 0 || (priority & 0xFFFCFFFF) != 0)) return false; //KernelBase.SetThreadPriority checks

            if (priority == THREAD_BASE_PRIORITY_LOWRT) {
                priority++;
            }
            else if (priority == THREAD_BASE_PRIORITY_IDLE) {
                priority--;
            }

            auto status = NtSetInformationThread(handle, THREADINFOCLASS::ThreadBasePriority, &priority, sizeof(ui32_t));

            return NT_SUCCESS(status);
        }

    protected:
        using handle_t = handle::native_handle_t;
        using thread_procedure_t = void(*)(void*);
        using thread_start_t = PTHREAD_START_ROUTINE;

        id_t _id;
        handle_t _handle;

        __forceinline constexpr thread(id_t id, handle_t handle) noexcept {
            _id = id;
            _handle = handle;
        }

        __forceinline handle_t temp_handle(id_t id, const handle_t& source, ui32_t open_access) const noexcept {
            auto result = (handle_t&)source;
            if (!source) {
                (result = handle_t(get_handle(id, open_access), __threadHandleCloser, false)).close_on_destroy(true);
            }

            return result;
        }

        __forceinline bool set_suspended(bool state) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, THREAD_SUSPEND_RESUME);
            if (!handle) {
            _Exit:
                return result;
            }

            auto procedure = state ? NtSuspendThread : NtResumeThread;
            result = NT_SUCCESS(procedure(handle.get(), nullptr));

            goto _Exit;
        }

        static __forceinline handle::native_t create_ex(handle::native_t process, address_t start, address_t parameter, unsigned flags, size_t stack_size) noexcept {
            auto handle = handle::native_t();

            if (!process) {
                process = __current_process;
            }

            auto status = NtCreateThreadEx(&handle, THREAD_ALL_ACCESS, nullptr, process, start, parameter, flags, null, stack_size, null, nullptr);
#ifdef NCORE_THREAD_EVENT_CREATION_FAILURE
            if (NT_ERROR(status)) {
                NCORE_THREAD_EVENT_CREATION_FAILURE;
            }
#endif

            return handle;
        }

    public:
        __forceinline constexpr thread(id_t id = null, ui32_t open_access = null) noexcept {
            if ((_id = id) && open_access) {
                _handle = handle_t(get_handle(id, open_access), __threadHandleCloser, false);
            }
        }

        __forceinline thread(handle::native_t win32_handle) noexcept {
            _id = get_id(win32_handle);
            _handle = handle_t(win32_handle, __threadHandleCloser, false);
        }

        static __forceinline thread current(unsigned open_access = THREAD_ALL_ACCESS) noexcept {
            return open_access ? thread(__thread_id, open_access) : thread(__current_thread);
        }

        static __forceinline thread get_by_id(id_t id, unsigned open_access = null) noexcept {
            return thread(id, open_access);
        }

        static __forceinline thread create(address_t start, void* parameter = nullptr, handle::native_t process = nullptr, bool keep_handle = false, int priority = null, unsigned flags = null, size_t stack_size = null) noexcept {
            auto handle = create_ex(process, start, parameter, flags, stack_size);
            if (!handle) return thread();

            if (priority) {
                set_priority(handle, priority);
            }

            if (keep_handle) return thread(handle);

            auto id = get_id(handle);
            __threadHandleCloser(handle);

            return thread(id);
        }

        template <class procedure_t, class... parameters_t> static __forceinline thread invoke(std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>* _result, procedure_t&& procedure, parameters_t&&... parameters) noexcept {
            auto invoker = ncore::invoker::make<true>(_result, std::forward<procedure_t>(procedure), std::forward<parameters_t>(parameters)...);

            auto handle = create_ex(nullptr, invoker.procedure(), invoker.parameters(), null, null);
            if (!handle) return invoker.release(), thread();

            auto id = get_id(handle);
            __threadHandleCloser(handle);

            return thread(id);
        }

        __forceinline id_t id() const noexcept {
            return _id;
        }

        __forceinline auto handle() const noexcept {
            return _handle.get();
        }

        __forceinline auto handle(ui32_t open_access = __defaultThreadOpenAccess) noexcept {
            return _handle.get() ? _handle.get() : (_handle = handle::native_handle_t(get_handle(_id, open_access), __threadHandleCloser)).get();
        }

        __forceinline auto close_handle() noexcept {
            return _handle.close();
        }

        __forceinline auto release() noexcept {
            return _handle.close();
        }

        __forceinline auto get_exit_code() const noexcept {
            auto result = ui32_t();

            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            if (!handle) _Exit: return result;

            result = get_info(handle.get()).ExitStatus;

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

            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION | SYNCHRONIZE);
            if (!handle) return false;

            *_status = get_info(handle.get()).ExitStatus;

            return *_status == STATUS_PENDING && WaitForSingleObject(handle.get(), null) != WAIT_OBJECT_0;
        }

        __forceinline auto suspend() const noexcept {
            return set_suspended(true);
        }

        __forceinline auto resume() const noexcept {
            return set_suspended(false);
        }

        __forceinline auto terminate(long exit_status = EXIT_SUCCESS) const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_TERMINATE);
            return handle.get() ? NT_SUCCESS(NtTerminateThread(handle.get(), exit_status)) : false;
        }

        __forceinline auto set_priority(ui32_t priority) const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            return bool(handle.get() ? set_priority(handle.get(), priority) : false);
        }

        __forceinline auto set_context(const context_t& context) const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            return handle.get() ? NT_SUCCESS(NtSetContextThread(handle.get(), PCONTEXT(&context))) : false;
        }

        __forceinline bool set_start_address(address_t address = nullptr) const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            return handle.get() ?
                NT_SUCCESS(NtSetInformationThread(handle.get(), THREADINFOCLASS::ThreadQuerySetWin32StartAddress, &address, sizeof(address_t))) :
                false;
        }

        __forceinline auto get_priority() const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            return get_priority(handle.get());
        }

        __forceinline auto get_process() const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            return get_process(handle.get());
        }

        __forceinline auto get_context() const noexcept {
            auto result = context_t();
            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            return handle.get() && NT_SUCCESS(NtGetContextThread(handle.get(), &result)) ? result : context_t();
        }

        __forceinline address_t get_start_address() const noexcept {
            auto result = address_t();
            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            return handle.get() && NT_SUCCESS(NtQueryInformationThread(handle.get(), THREADINFOCLASS::ThreadQuerySetWin32StartAddress, &result, sizeof(address_t), null)) ? result : address_t();
        }

        template<typename _t = address_t> __forceinline _t get_environment_address() const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            if (!handle) return (_t)nullptr;

            auto information = THREAD_BASIC_INFORMATION();
            auto information_length = ULONG(sizeof(information));
            NtQueryInformationThread(handle.get(), THREADINFOCLASS::ThreadBasicInformation, &information, information_length, &information_length);

            return (_t)information.TebBaseAddress;
        }

        __forceinline auto get_environment() const noexcept {
            auto result = TEB();

            auto environment_address = get_environment_address();
            if (!environment_address) _Exit: return result;

            auto process_id = get_process();
            if (!process_id) goto _Exit;

            auto process_handle = handle::native_t(null);
            auto attributes = OBJECT_ATTRIBUTES();
            auto client_id = CLIENT_ID();

            client_id.UniqueThread = handle::native_t(null);
            client_id.UniqueProcess = handle::native_t(process_id);
            InitializeObjectAttributes(&attributes, null, null, null, null);

            if (NT_ERROR(NtOpenProcess(&process_handle, PROCESS_VM_OPERATION | PROCESS_VM_READ, &attributes, &client_id))) goto _Exit;

            NtReadVirtualMemory(process_handle, environment_address, &result, sizeof(result), nullptr);

            NtClose(process_handle);

            goto _Exit;
        }

        static __forceinline void set_timer_resolution(ui32_t time) noexcept {
            NtSetTimerResolution(time, true, nullptr);
        }

        static __forceinline void sleep_micro(i32_t microsecounds, bool alertable = false) noexcept {
            if (microsecounds < null) return;

            auto delay = __lit_micro(microsecounds);
            NtDelayExecution(alertable, &delay);
        }

        static __forceinline void sleep_mili(i32_t milisecounds, bool alertable = false) noexcept {
            return sleep_micro(milisecounds * 1000, alertable);
        }

        static __forceinline void sleep(i32_t time, bool alertable = false) noexcept {
#ifdef NCORE_THREAD_SLEEP_MICRO
            return sleep_micro(time, alertable);
#else
            return sleep_mili(time, alertable);
#endif
        }
    };
}