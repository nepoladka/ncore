#pragma once
#include "defines.hpp"
#include "static_array.hpp"
#include "handle.hpp"
#include <windows.h>
#include <string>
#include "includes/ntos.h"

namespace ncore {
    using namespace std;

    static const handle::native_handle_t::closer_t const __threadHandleCloser = handle::native_handle_t::closer_t(NtClose);

    class thread {
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
            NtCreateThreadEx(&handle, THREAD_ALL_ACCESS, nullptr, process, start, parameter, flags, null, stack_size, null, nullptr);
            return handle;
        }

    public:
        __forceinline thread(id_t id = null, unsigned open_access = null) noexcept {
            if ((_id = id) && open_access) {
                _handle = handle_t(OpenThread(id, false, open_access), __threadHandleCloser, false);
            }
        }

        __forceinline thread(handle::native_t win32_handle) noexcept {
            _id = GetThreadId(win32_handle);
            _handle = handle_t(win32_handle, __threadHandleCloser, false);
        }

        static __forceinline thread current() noexcept {
            return thread(GetCurrentThreadId());
        }

        static __forceinline thread get_by_id(id_t id) noexcept {
            return thread(id);
        }

        static __forceinline thread create(address_t start, void* parameter = nullptr, handle::native_t process = nullptr, bool keep_handle = false, int priority = null, unsigned flags = null, size_t stack_size = null) noexcept {
            if (!start) _Fail: return thread();
            
            if (!process) {
                process = CURRENT_PROCESS_HANDLE;
            }

            auto handle = create_ex(process, start, parameter, flags, stack_size);
            if (!handle) goto _Fail;

            if (priority) {
                SetThreadPriority(handle, priority);
            }

            if(keep_handle) return thread(handle);

            auto id = id_t(GetThreadId(handle));
            __threadHandleCloser(handle);

            return thread(id);
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

        __forceinline address_t start_address() const noexcept {
            auto result = address_t(nullptr);

            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            NtQueryInformationThread(handle.get(), THREADINFOCLASS::ThreadQuerySetWin32StartAddress, &result, sizeof(address_t), null);

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

        __forceinline int get_priority() const noexcept {
            auto result = null;

            auto handle = temp_handle(_id, _handle, THREAD_QUERY_INFORMATION);
            if (!handle) {
            _Exit:
                return result;
            }

            result = GetThreadPriority(handle.get());

            goto _Exit;
        }

        static __forceinline void set_timer_resolution(ui32_t time) {
            NtSetTimerResolution(time, true, nullptr);
        }

        static __forceinline void sleep(i32_t milisecounds) {
            if (milisecounds < null) return;

            auto interval = LARGE_INTEGER();
            interval.QuadPart = -10000 * milisecounds;
            NtDelayExecution(false, &interval);
        }
    };
    
    class named_thread : public thread {
    public:
        string name;

        __forceinline named_thread(const thread& base, const string& name) : 
            thread(((named_thread*)(&base))->_id, ((named_thread*)(&base))->_handle) {
            this->name = name;
        }
    };

    template <class _specificThreadInfo> class multi_thread_info {
    public:
        class specific_thread_info : public _specificThreadInfo {
        public:
            thread thread;
        };

        bool release_after_using = false;
        size_t threads_count = null;
        size_t alive_threads_count = null;
        specific_thread_info* threads = null;

        __forceinline multi_thread_info(bool release_after_using) {
            this->release_after_using = release_after_using;
        }

        __forceinline void alloc(size_t threads_count) {
            auto previous_threads = threads;
            threads = new specific_thread_info[this->threads_count = threads_count];

            if (previous_threads) return delete[] previous_threads;
        }

        __forceinline void release() {
            if (threads) return delete[] threads;
        }

        __forceinline void abort() {
            for (size_t i = 0; i < threads_count; i++)
                threads[i].thread.terminate();
        }

        __forceinline void suspend() {
            for (size_t i = 0; i < threads_count; i++)
                threads[i].thread.suspend();
        }

        __forceinline void resume() {
            for (size_t i = 0; i < threads_count; i++)
                threads[i].thread.resume();
        }
    };
}