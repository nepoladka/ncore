#pragma once
#include "defines.hpp"
#include "handle.hpp"
#include "static_array.hpp"
#include "environment.hpp"

#include <string>
#include <tuple>

/* available defines:
    NCORE_THREAD_EVENT_CREATION_FAILURE
*/

namespace ncore {
    static const auto const __threadHandleCloser = handle::native_handle_t::closer_t(NtClose);

    class thread {
    public:
        using context_t = CONTEXT;

        static __forceinline auto get_info(handle::native_t handle) noexcept {
            using info_t = THREAD_BASIC_INFORMATION;

            auto information = info_t();
            if (handle) {
                auto information_length = ULONG(sizeof(information));
                NtQueryInformationThread(handle, THREADINFOCLASS::ThreadBasicInformation, &information, information_length, &information_length);
            }

            return information;
        }

        static __forceinline auto get_id(handle::native_t handle) noexcept {
            return id_t(get_info(handle).ClientId.UniqueThread);
        }

        static __forceinline auto get_process(handle::native_t handle) noexcept {
            return id_t(get_info(handle).ClientId.UniqueProcess);
        }

        static __forceinline auto get_priority(handle::native_t handle) noexcept {
            return ui32_t(get_info(handle).BasePriority);
        }

    protected:
        using handle_t = handle::native_handle_t;
        using thread_procedure_t = void(*)(void*);
        using thread_start_t = PTHREAD_START_ROUTINE;

        id_t _id;
        handle_t _handle;

        __forceinline thread(id_t id, handle_t handle) noexcept {
            _id = id;
            _handle = handle;
        }

        __forceinline handle_t temp_handle(id_t id, const handle_t& source, unsigned open_access) const noexcept {
            auto result = (handle_t&)source;
            if (!source) {
                (result = handle_t(OpenThread(open_access, false, id), __threadHandleCloser, false)).close_on_destroy(true);
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

    private:
        struct invoke_info {
            void* tuple;
            void** result;
        };

        template <class result_t, class tuple_t, index_t... _indexes> static __declspec(noinline) void invoker(invoke_info* data) noexcept {
            const std::unique_ptr<tuple_t> values((tuple_t*)data->tuple);
            tuple_t& tuple = *values.get();

            if constexpr (std::is_same<result_t, void**>::value) {
                std::invoke(std::move(std::get<_indexes>(tuple))...);
            }
            else {
                auto result = std::invoke(std::move(std::get<_indexes>(tuple))...);
                if (data->result && *data->result) {
                    **((decltype(result)**)(data->result)) = result;
                }
            }

            return delete data;
        }

        template <class result_t, class tuple_t, index_t... _indexes> static __forceinline constexpr auto invoker_for(std::index_sequence<_indexes...>) noexcept {
            return &invoker<result_t, tuple_t, _indexes...>;
        }

    public:
        __forceinline thread(id_t id = null, unsigned open_access = null) noexcept {
            if ((_id = id) && open_access) {
                _handle = handle_t(OpenThread(id, false, open_access), __threadHandleCloser, false);
            }
        }

        __forceinline thread(handle::native_t win32_handle) noexcept {
            _id = get_id(win32_handle);
            _handle = handle_t(win32_handle, __threadHandleCloser, false);
        }

        __forceinline id_t id() const noexcept {
            return _id;
        }

        __forceinline handle::native_t handle() const noexcept {
            return _handle.get();
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
                SetThreadPriority(handle, priority);
            }

            if (keep_handle) return thread(handle);

            auto id = get_id(handle);
            __threadHandleCloser(handle);

            return thread(id);
        }

        template <class procedure_t, class... parameters_t> static __forceinline thread invoke(std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>** _result, procedure_t&& procedure, parameters_t&&... parameters) noexcept {
            using tuple_t = std::tuple<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;

            auto decay = std::make_unique<tuple_t>(std::forward<procedure_t>(procedure), std::forward<parameters_t>(parameters)...);
            constexpr auto invoker = invoker_for<decltype(_result), tuple_t>(std::make_index_sequence<1 + sizeof...(parameters_t)>{});

            auto data = new invoke_info{ decay.get(), (void**)_result };

            auto handle = create_ex(nullptr, invoker, data, null, null);
            if (!handle) {
                delete data;
                return thread();
            }

            decay.release();

            auto id = get_id(handle);
            __threadHandleCloser(handle);

            return thread(id);
        }

        __forceinline void close_handle() noexcept {
            return _handle.close();
        }

        __forceinline unsigned wait(unsigned milisecounds_timeout = INFINITE) const noexcept {
            auto result = 0ui32;

            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            if (!handle) {
            _Exit:
                return result;
            }

            result = WaitForSingleObject(handle.get(), milisecounds_timeout);

            goto _Exit;
        }

        __forceinline bool alive(unsigned* _exit_code = nullptr) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            result = (WaitForSingleObject(handle.get(), null) != WAIT_OBJECT_0);

            if (_exit_code) {
                GetExitCodeThread(handle.get(), (LPDWORD)(_exit_code));
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

            auto handle = temp_handle(_id, _handle, THREAD_TERMINATE);
            if (!handle) {
            _Exit:
                return result;
            }

            result = NT_SUCCESS(NtTerminateThread(handle.get(), exit_status));

            goto _Exit;
        }

        __forceinline bool set_priority(int priority) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            if (!handle) {
            _Exit:
                return result;
            }

            result = SetThreadPriority(handle.get(), priority);

            goto _Exit;
        }

        __forceinline bool set_context(const context_t& context) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            if (!handle) {
            _Exit:
                return result;
            }

            result = NT_SUCCESS(NtSetContextThread(handle.get(), PCONTEXT(&context)));

            goto _Exit;
        }

        __forceinline bool set_start_address(address_t address = nullptr) const noexcept {
            auto result = false;

            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            if (!handle) {
            _Exit:
                return result;
            }

            result = NT_SUCCESS(NtSetInformationThread(handle.get(), THREADINFOCLASS::ThreadQuerySetWin32StartAddress, &address, sizeof(address_t)));

            goto _Exit;
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
            if (!handle) {
            _Exit:
                return result;
            }

            NtGetContextThread(handle.get(), &result);

            goto _Exit;
        }

        __forceinline address_t get_start_address() const noexcept {
            auto result = address_t(nullptr);

            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            NtQueryInformationThread(handle.get(), THREADINFOCLASS::ThreadQuerySetWin32StartAddress, &result, sizeof(address_t), null);

            goto _Exit;
        }

        template<typename _t = address_t> __forceinline _t get_environment_address() const noexcept {
            auto handle = temp_handle(_id, _handle, THREAD_ALL_ACCESS);
            if (!handle) return nullptr;

            auto information = THREAD_BASIC_INFORMATION();
            auto information_length = ULONG(sizeof(information));
            NtQueryInformationThread(handle.get(), THREADINFOCLASS::ThreadBasicInformation, &information, information_length, &information_length);

            return (_t)information.TebBaseAddress;
        }

        __forceinline TEB get_environment() const noexcept {
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

        static __forceinline void sleep_micro(i32_t microsecounds) noexcept {
            if (microsecounds < null) return;

            auto interval = LARGE_INTEGER();
            interval.QuadPart = -10 * microsecounds;
            NtDelayExecution(false, &interval);
        }

        static __forceinline void sleep_mili(i32_t milisecounds) noexcept {
            return sleep_micro(milisecounds * 1000);
        }

        static __forceinline void sleep(i32_t time) noexcept {
#ifdef NCORE_THREAD_SLEEP_MICRO
            return sleep_micro(time);
#else
            return sleep_mili(time);
#endif
        }
    };
}