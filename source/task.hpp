#pragma once
#include "thread.hpp"

#define noarg (void)
#define __await(TASK) ((TASK).wait())
#define __task(...) (ncore::async(__VA_ARGS__))

namespace ncore {
	template <typename _t> class task {
	public:
		using result_t = _t;

	private:
		thread _thread;
		result_t* _result;

	public:
		__forceinline task() = default;

		template <class procedure_t, class... parameters_t> static __forceinline task create(procedure_t&& procedure, parameters_t&&... parameters) noexcept {
			auto result = task();
			
			if constexpr (std::is_same<result_t, void>::value) {
				result._result = nullptr;
			}
			else {
				result._result = new result_t();
			}

			result._thread = thread::invoke(&result._result, std::forward<procedure_t>(procedure), std::forward<parameters_t>(parameters)...);

			return result;
		}

		static __forceinline task attach(const thread& thread) noexcept {
			auto result = task();
			result._thread = thread;
			result._result = nullptr;
			return result;
		}

		__forceinline ~task() noexcept {
			if constexpr (std::is_same<result_t, void>::value) return;

			auto result = _result;
			if (!result) return;

			_result = nullptr;
			delete result;
		}

		__forceinline constexpr auto& thread() noexcept {
			return _thread;
		}

		__forceinline constexpr const auto& thread() const noexcept {
			return _thread;
		}

		__forceinline auto alive() const noexcept {
			return _thread.alive();
		}

		__forceinline auto ready() const noexcept {
			return !_thread.alive();
		}

		__forceinline auto& wait() noexcept {
			_thread.wait();
			return *this;
		}

		__forceinline const auto& wait() const noexcept {
			_thread.wait();
			return *this;
		}

		__forceinline constexpr auto has_result() const noexcept {
			return _result != nullptr;
		}

		__forceinline auto get() const noexcept {
			_thread.wait();

			if constexpr (std::is_same<result_t, void>::value) return;
			else return _result ? *_result : result_t();
		}
	};

	template <class procedure_t, class... parameters_t> static __forceinline auto async(procedure_t&& procedure, parameters_t&&... parameters) noexcept {
		return task<std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>>::create(std::forward<procedure_t>(procedure), std::forward<parameters_t>(parameters)...);
	}
}
