#pragma once
#include "thread.hpp"

namespace ncore {
	template <typename _t> class task {
	public:
		using result_t = _t;

	private:
		thread _thread;
		result_t* _result;

	public:
		__forceinline constexpr task() = default;

		//do not forget call task::release to destroy the object and release memory
		template <class procedure_t, class... parameters_t> static __forceinline task create(procedure_t&& procedure, parameters_t&&... parameters) noexcept {
			auto task = ncore::task<result_t>();

			if constexpr (std::is_same<result_t, void>::value) {
				task._result = nullptr;
			}
			else {
				task._result = new result_t();
			}

			task._thread = thread::invoke(task._result, procedure, std::forward<parameters_t>(parameters)...);

			return task;
		}

		static __forceinline task attach(const thread& thread, result_t* buffer = nullptr) noexcept {
			auto result = task();
			result._thread = thread;
			result._result = buffer;
			return result;
		}

		__forceinline void release() noexcept {
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

		__forceinline constexpr auto result(bool wait = true) const noexcept {
			if (wait) {
				_thread.wait();
			}

			if constexpr (std::is_same<result_t, void>::value) {
				return;
			}
			else {
				return _result ? *_result : result_t();
			}
		}

		__forceinline constexpr auto get() const noexcept {
			return result();
		}
	};

	template <class procedure_t, class... parameters_t> static __forceinline auto async(procedure_t&& procedure, parameters_t&&... parameters) noexcept {
		using result_t = std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;
		return task<result_t>::create(procedure, std::forward<parameters_t>(parameters)...);
	}

	template<class result_t> static __forceinline auto wait_multi(const std::vector<task<result_t>>& tasks, ui32_t delay = 500) noexcept {
		auto alive = count_t();
		do {
			thread::sleep(delay);

			alive = null;
			for (auto& task : tasks) {
				alive += task.alive();
			}
		} while (alive);
	}
}
