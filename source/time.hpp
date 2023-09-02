#pragma once
#include "defines.hpp"
#include <windows.h>
#include <time.h>

namespace ncore {
    using file_time_t = FILETIME;
    using system_time_t = SYSTEMTIME;

    class time : private system_time_t {
    private:
        static __forceinline bool time_of(HANDLE handle, bool thread, time* _creation, time* _exit, time* _kernel, time* _user) noexcept {
            file_time_t creation, exit, kernel, user;

            using get_time_of_t = BOOL(*)(HANDLE, FILETIME*, FILETIME*, FILETIME*, FILETIME*);
            static const get_time_of_t get_time_of[] = {
                GetProcessTimes,
                GetThreadTimes };

            if (!get_time_of[thread](handle, &creation, &exit, &kernel, &user)) return false;

            if (_creation) {
                *_creation = time(creation);
            }

            if (_exit) {
                *_exit = time(exit);
            }

            if (_kernel) {
                *_kernel = time(kernel);
            }

            if (_user) {
                *_user = time(user);
            }

            return true;
        }

    public:
        __forceinline time() = default;

        __forceinline time(const file_time_t& file_time) {
            FileTimeToSystemTime(&file_time, this);
        }

        __forceinline time(const system_time_t& system_time) {
            *this = system_time;
        }

        static __forceinline time current_system() noexcept {
            time result;
            GetSystemTime(&result);
            return result;
        }

        static __forceinline time current() noexcept {
            time result;
            GetLocalTime(&result);
            return result;
        }

        static __forceinline bool times_of_process(HANDLE process, time* _creation = nullptr, time* _exit = nullptr, time* _kernel = nullptr, time* _user = nullptr) noexcept {
            return time_of(process, false, _creation, _exit, _kernel, _user);
        }

        static __forceinline bool times_of_thread(HANDLE thread, time* _creation = nullptr, time* _exit = nullptr, time* _kernel = nullptr, time* _user = nullptr) noexcept {
            return time_of(thread, true, _creation, _exit, _kernel, _user);
        }

        __forceinline constexpr ui16_t year() const noexcept {
            return wYear;
        }

        __forceinline constexpr ui16_t month() const noexcept {
            return wMonth;
        }

        __forceinline constexpr ui16_t day() const noexcept {
            return wDay;
        }

        __forceinline constexpr ui16_t hour() const noexcept {
            return wHour;
        }

        __forceinline constexpr ui16_t minute() const noexcept {
            return wMinute;
        }

        __forceinline constexpr ui16_t secound() const noexcept {
            return wSecond;
        }

        __forceinline constexpr ui16_t millisecond() const noexcept {
            return wMilliseconds;
        }

        __forceinline constexpr ui16_t day_of_week() const noexcept {
            return wDayOfWeek;
        }

        __forceinline constexpr ui32_t secounds_of_minute() const noexcept {
            return minute() * 60 + secound();
        }

        __forceinline constexpr ui32_t secounds_of_hour() const noexcept {
            return hour() * 60 * 60 + secounds_of_minute();
        }

        __forceinline constexpr ui32_t secounds_of_day() const noexcept {
            return day() * 24 * 60 * 60 + secounds_of_hour();
        }

        __forceinline constexpr ui32_t milisecounds_of_secound() const noexcept {
            return secound() * 1000 + millisecond();
        }

        __forceinline constexpr ui32_t milisecounds_of_minute() const noexcept {
            return minute() * 60 * 1000 + milisecounds_of_secound();
        }

        __forceinline constexpr ui32_t milisecounds_of_hour() const noexcept {
            return hour() * 60 * 60 * 1000 + milisecounds_of_minute();
        }

        __forceinline constexpr ui64_t milisecounds_of_day() const noexcept {
            return day() * 24 * 60 * 60 * 1000 + milisecounds_of_hour();
        }

        __forceinline constexpr ui64_t milisecounds_of_month(ui8_t days = 30) const noexcept {
            return month() * days * 24 * 60 * 60 * 1000 + milisecounds_of_day();
        }


        __forceinline file_time_t to_file_time() const noexcept {
            file_time_t result;
            SystemTimeToFileTime(this, &result);
            return result;
        }

        __forceinline constexpr system_time_t to_system_time() const noexcept {
            return *(system_time_t*)this;
        }
    };
}