#pragma once
#include "defines.hpp"

#include <tuple>
#include <memory>
#include <type_traits>
#include <forward_list>

#define __tuple(TYPE, NAME) TYPE... NAME
#define __tuple_head(NAME) __tuple(typename, NAME)
#define __tuple_pass(TYPE, NAME) std::forward<TYPE>(NAME)...
#define __tuple_invoke(PROCEDURE, TYPE, NAME) PROCEDURE<TYPE...>(__tuple_pass(TYPE, NAME))

#pragma warning(push)
#pragma warning(disable : 0417)
#pragma warning(disable : 1919)

namespace ncore {
    template<typename _t> struct return_of;

    template<typename ret_t, typename... args_t> struct return_of<ret_t(args_t...)> {
        using type = ret_t;
    };

	template<typename return_t> class action {
	public:
		template<typename... parameters_t> using procedure_t = return_t(*)(parameters_t...);

	protected:
		address_t _address;

        template <typename... parameters_t, typename tuple_t, index_t... _indexes> __forceinline constexpr return_t invoke_helper(tuple_t&& arguments, std::index_sequence<_indexes...>) noexcept {
            return _address ? procedure_t<parameters_t...>(_address)(std::get<_indexes>(std::forward<tuple_t>(arguments))...) : return_t();
        }

	public:
		__forceinline constexpr action(address_t address = { }) noexcept {
			_address = address;
		}

		template<typename _t> __forceinline constexpr action(const _t& procedure) noexcept {
			_address = address_t(procedure);
		}

		template<typename... parameters_t> __forceinline constexpr return_t invoke(parameters_t&&... arguments) noexcept {
			return invoke_helper<parameters_t...>(std::forward_as_tuple(std::forward<parameters_t>(arguments)...), std::index_sequence_for<parameters_t...>{ });
		}

		template<typename... parameters_t> __forceinline constexpr return_t operator()(parameters_t&&... arguments) noexcept {
			return invoke_helper<parameters_t...>(std::forward_as_tuple(std::forward<parameters_t>(arguments)...), std::index_sequence_for<parameters_t...>{ });
		}
	};

    class invoker {
    public:
        template<class return_t, class tuple_t> struct invoke_info {
            return_t* result;
            tuple_t tuple;

            __forceinline constexpr invoke_info(return_t* result) noexcept {
                invoke_info::result = result;
                invoke_info::tuple = { };
            }
        };

    protected:
        address_t _source;
        address_t _procedure;
        address_t _parameters;

        template <bool _dispose, class return_t, class tuple_t, index_t... _indexes> static __declspec(noinline) void procedure(invoke_info<return_t, tuple_t>* data) noexcept {
            if (!data) return;
            
            auto& tuple = data->tuple;

            if constexpr (std::is_same<return_t, void>::value) {
                std::invoke(std::move(std::get<_indexes>(tuple))...);
            }
            else {
                auto result = std::invoke(std::move(std::get<_indexes>(tuple))...);
                if (data->result) {
                    *data->result = result;
                }
            }

            if constexpr (_dispose) {
                delete data;
            }
        }

        template <bool _dispose, class return_t, class tuple_t, index_t... _indexes> static __forceinline constexpr auto invoker_for(std::index_sequence<_indexes...>) noexcept {
            return &procedure<_dispose, return_t, tuple_t, _indexes...>;
        }

    public:
        __forceinline constexpr invoker(address_t source = { }, address_t procedure = { }, address_t parameters = { }) noexcept {
            _source = source;
            _procedure = procedure;
            _parameters = parameters;
        }

        __forceinline constexpr ~invoker() = default;

        template <bool _dispose, class procedure_t, class... parameters_t> static __forceinline constexpr auto make_procedure(procedure_t&& procedure, parameters_t&&... parameters) noexcept {
            using result_t = std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;
            using tuple_t = std::tuple<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;

            return invoker_for<_dispose, result_t, tuple_t>(std::make_index_sequence<1 + sizeof...(parameters_t)>{});
        }

        template <bool _store, class procedure_t, class... parameters_t> static __forceinline constexpr auto make_parameters(std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>* _result, procedure_t&& procedure, parameters_t&&... parameters) noexcept {
            using result_t = std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;
            using tuple_t = std::tuple<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;

            if constexpr (_store) {
                auto info = new invoke_info<result_t, tuple_t>(_result);
                info->tuple = tuple_t(std::forward<procedure_t>(procedure), std::forward<parameters_t>(parameters)...);
                return info;
            }
            else {
                auto info = invoke_info<result_t, tuple_t>(_result);
                info.tuple = tuple_t(std::forward<procedure_t>(procedure), std::forward<parameters_t>(parameters)...);
                return info;
            }
        }

        template <bool _dispose, class procedure_t, class... parameters_t> static __forceinline constexpr auto make(std::_Invoke_result_t<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>* _result, procedure_t&& procedure, parameters_t&&... parameters) noexcept {
            auto result = invoker(); {
                result._source = address_t(&procedure);
                result._procedure = make_procedure<_dispose>(procedure, std::forward<parameters_t>(parameters)...);
                result._parameters = make_parameters<true>(_result, procedure, std::forward<parameters_t>(parameters)...);
            }

            return result;
        }

        template <bool _store, class result_t, class... parameters_t> __forceinline constexpr auto remake(result_t* _result, parameters_t&&... parameters) const noexcept {
            using procedure_t = void(*)(parameters_t...);
            using tuple_t = std::tuple<std::decay_t<procedure_t>, std::decay_t<parameters_t>...>;

            if constexpr (_store) {
                auto info = new invoke_info<result_t, tuple_t>(_result);

                info->tuple = tuple_t(std::forward<procedure_t>(procedure_t(_source)), std::forward<parameters_t>(parameters)...);

                return info;
            }
            else {
                auto info = invoke_info<result_t, tuple_t>(_result);

                info.tuple = tuple_t(std::forward<procedure_t>(procedure_t(_source)), std::forward<parameters_t>(parameters)...);

                return info;
            }
        }

        __forceinline constexpr void apply(address_t info) noexcept {
            _parameters = info;
        }

        __forceinline constexpr auto release() noexcept {
            if (auto parameters = _parameters) {
                _parameters = nullptr;
                delete parameters;
            }
        }

        template<typename _t = address_t> __forceinline constexpr const auto procedure() const noexcept {
            return (_t)_procedure;
        }

        template<typename _t = address_t> __forceinline constexpr const auto parameters() const noexcept {
            return (_t)_parameters;
        }

        template<typename _t> __forceinline constexpr auto result() const noexcept {
            if constexpr (std::is_same<_t, void>::value) {
                return;
            }
            else {
                auto info = (invoke_info<_t, typeof(nullptr)>*)_parameters;
                if (!info) return _t();

                auto result = info->result;
                if (!result) return _t();

                return *result;
            }
        }

        template<typename _t = void> __forceinline constexpr auto invoke() const noexcept {
            return ((void(*)(address_t))_procedure)(_parameters), result<_t>();
        }

        template <class _t, class... parameters_t> __forceinline constexpr auto invoke(parameters_t&&... parameters) const noexcept {
            if constexpr (std::is_same<_t, void>::value) {
                auto info = remake<false, void>(nullptr, std::forward<parameters_t>(parameters)...);

                return ((void(*)(address_t))_procedure)(&info);
            }
            else {
                auto result = _t();
                auto info = remake<false, _t>(&result, std::forward<parameters_t>(parameters)...);

                return ((void(*)(address_t))_procedure)(&info), result;
            }
        }

        __forceinline constexpr auto operator()() const noexcept {
            return ((void(*)(address_t))_procedure)(_parameters);
        }

        template <class... parameters_t> __forceinline constexpr auto operator()(parameters_t&&... parameters) const noexcept {
            auto info = remake<false, void>(nullptr, std::forward<parameters_t>(parameters)...);

            return ((void(*)(address_t))_procedure)(&info);
        }

        __forceinline constexpr operator bool() const noexcept {
            return _source && _procedure && _parameters;
        }
    };

	/*template<typename _t, __tuple_head(arguments_t)> __forceinline constexpr _t invoke_action(action<_t>&& action, __tuple(arguments_t&&, arguments)) noexcept {
		return __tuple_invoke(action.invoke, arguments_t, arguments);
	}

	// ^^^ that equals this ->

	template<typename _t, typename... arguments_t> __forceinline constexpr _t invoke_action(action<_t>&& action, arguments_t&&... arguments) noexcept {
		return action.invoke<arguments_t...>(std::forward<arguments_t>(arguments)...);
	}
	*/
}

#pragma warning(pop)
