module;

#include <windows.h>
#include <debugapi.h>

export module Result;

export import <string>;
import <optional>;
import <type_traits>;
import <concepts>;
import Variant;
import <coroutine>;
import <format>;

export namespace ji {
	namespace exception {
		class no_error : public std::exception {
		public:
			const char* what() const override {
				return "trying to access error at result which is not in error state";
			}
		};

		template <typename T>
		class no_value : public std::exception {
		public:
			no_value(const T& error) :
				//std::exception(getErrorMessage(error)),
				std::exception("no value"),
				m_error(error) { }

			no_value(T&& error) :
				//ftm::exception(getErrorMessage(error)),
				std::exception("no value"),
				m_error(std::move(error)) { }

			[[nodiscard]] const T& error() const& { return m_error; }
			[[nodiscard]] T&& error()&& { return std::move(m_error); }

		private:
			T m_error;
			std::string m_message;
		};
	}

	template<typename T>
	struct error {
		error(const T& v) noexcept(std::is_nothrow_constructible_v<T, decltype(v)>) : val(v) {}
		error(T&& v) noexcept(std::is_nothrow_constructible_v<T, decltype(v)>) : val(std::move(v)) {}

		T val;
	};

	struct ok {};

	template<typename T>
	struct as_is {
		as_is(const T& v) noexcept(std::is_nothrow_constructible_v<T, decltype(v)>) : val(v) {}
		as_is(T&& v) noexcept(std::is_nothrow_constructible_v<T, decltype(v)>) : val(std::move(v)) {}

		T val;
	};

	template<class OkVal, class ErrorVal>
	class result {
	private:
		result() noexcept = default;

		template<typename T>
		static void raise_exception(const T& error) {
			throw exception::no_value<T>(error);
		}

		template<typename... T>
		static void raise_exception(const std::variant<T...>& var) {
			ji::match(var, ji::cases{
				[](const auto& error) {
					raise_exception(error);
				}
			});
		}
	public:
		struct promise_type {
			result* m_result = nullptr;

			using ErrorType = ErrorVal;

			result get_return_object() noexcept {
				return result { *this };
			}

			std::suspend_never initial_suspend() const noexcept {
				return {};
			}

			std::suspend_never final_suspend() const noexcept {
				return {};
			}

			void return_value(result&& r) {
				*m_result = std::move(r);
			}

			void unhandled_exception() noexcept {}
		};

	private:
		struct awaiter {
			result m_result;

			bool await_ready() {
				return m_result.has_value();
			}

			OkVal await_resume() const& {
				return m_result.value();
			}

			OkVal await_resume()&& {
				return std::move(m_result).value();
			}

			template<typename T>
				requires std::constructible_from<ErrorVal, typename T::ErrorType&&>
			void await_suspend(std::coroutine_handle<T> handle) {
				*handle.promise().m_result = ji::error(std::move(m_result).error());
				handle.destroy();
			}
		};

		explicit result(promise_type& handle)
		{
			handle.m_result = this;
		}

		//std::coroutine_handle<promise_type> m_coro_handle;
	public:

		/*result(const result& e) {
			m_val = e.m_val;
			if (m_coro_handle) {
				m_coro_handle = e.m_coro_handle;
				m_coro_handle.promise().m_result = this;
			}
		}

		result(result&& e) noexcept {
			m_val = std::move(e.m_val);
			if (m_coro_handle) {
				m_coro_handle = e.m_coro_handle;
				m_coro_handle.promise().m_result = this;
			}
		}

		result& operator= (const result& e) {
			if (&e != this) {
				m_val = e.m_val;
				if (m_coro_handle) {
					m_coro_handle = e.m_coro_handle;
					m_coro_handle.promise().m_result = this;
				}
			}

			return *this;
		}

		result& operator= (result&& e) {
			if (&e != this) {
				m_val = std::move(e.m_val);
				if (m_coro_handle) {
					m_coro_handle = e.m_coro_handle;
					m_coro_handle.promise().m_result = this;
				}
			}

			return *this;
		}*/

		awaiter operator co_await() const & {
			return awaiter{ *this };
		}

		awaiter operator co_await() && {
			return awaiter{ std::move(*this) };
		}

		template<typename T>
			requires std::constructible_from<OkVal, const T&>
		result(const T& v) : m_val(v) {}

		template<typename T>
			requires std::constructible_from<OkVal, T&&>
		result(T&& v) : m_val(std::move(v)) {}

		template<typename T>
			requires std::constructible_from<ErrorVal, const T&>
		result(const ji::error<T>& v) : m_val(ji::error<ErrorVal>(v.val)) {}

		template<typename T>
			requires std::constructible_from<ErrorVal, T&&>
		result(ji::error<T>&& v) : m_val(ji::error<ErrorVal>(std::move(v.val))) {}

		template<typename T>
			requires std::constructible_from<ErrorVal, const T&>
		result(const ji::error<as_is<T>>& v) : m_val(ji::error<ErrorVal>(v.val.val)) {}

		template<typename T>
			requires std::constructible_from<ErrorVal, T&&>
		result(ji::error<as_is<T>>&& v) : m_val(ji::error<ErrorVal>(std::move(v.val.val))) {}

		template<typename... Args>
		result(const ji::error<std::variant<Args...>>& v) : m_val(ji::error<ErrorVal>(ji::match(v.val, ji::cases{
			[](const ErrorVal& err) { return err; }
		}))) { }

		template<typename... Args>
		result(ji::error<std::variant<Args...>>&& v) : m_val(ji::error<ErrorVal>(ji::match(std::move(v.val), ji::cases{
			[](ErrorVal&& err) { return std::forward<ErrorVal>(err); }
		}))) { }

		result& operator=(const OkVal& v) {
			m_val = v.m_val;
			return *this;
		}

		result& operator=(OkVal&& v) {
			m_val = std::move(v.m_val);
			return *this;
		}

		result& operator=(const error<ErrorVal>& v) {
			m_val = v.val;
			return *this;
		}

		result& operator=(error<ErrorVal>&& v) {
			m_val = std::move(v.val);
			return *this;
		}

		operator bool() const noexcept {
			return has_value();
		}

		[[nodiscard]] bool has_value() const noexcept {
			return std::holds_alternative<OkVal>(m_val);
		}

		[[nodiscard]] const OkVal& value() const& {
			if (const auto ok_ptr = std::get_if<OkVal>(&m_val)) {
				return *ok_ptr;
			}
			if (const auto error_ptr = std::get_if<ji::error<ErrorVal>>(&m_val)) {
				raise_exception(error_ptr->val);
			}
			throw std::exception("result is uninitialized");
		}

		[[nodiscard]] OkVal&& value()&& {
			if (const auto ok_ptr = std::get_if<OkVal>(&m_val)) {
				return std::move(*ok_ptr);
			}
			if (const auto error_ptr = std::get_if<ji::error<ErrorVal>>(&m_val)) {
				raise_exception(error_ptr->val);
			}
			throw std::exception("result is uninitialized");
		}

		template<typename E>
		[[nodiscard]] const OkVal& value_or_throw(E&& e)&& {
			if (const auto ok_ptr = std::get_if<OkVal>(&m_val)) {
				return std::move(*ok_ptr);
			}
			if (const auto error_ptr = std::get_if<ji::error<ErrorVal>>(&m_val)) {
				throw std::move(e);
			}
			throw std::exception("result is uninitialized");
		}

		[[nodiscard]] const ErrorVal& error() const& {
			if (const auto error_ptr = std::get_if<ji::error<ErrorVal>>(&m_val)) {
				return error_ptr->val;
			}

			throw exception::no_error{};
		}

		[[nodiscard]] ErrorVal&& error()&& {
			if (const auto error_ptr = std::get_if<ji::error<ErrorVal>>(&m_val)) {
				return std::move(error_ptr->val);
			}

			throw exception::no_error{};
		}

	private:
		std::variant<std::monostate, OkVal, ji::error<ErrorVal>> m_val;
	};

	template<class ErrorVal>
	class result<void, ErrorVal> {
	private:
		result() noexcept = default;

		template<typename T>
		static void raise_exception(const T& error) {
			throw exception::no_value<T>(error);
		}

		template<typename... T>
		static void raise_exception(const std::variant<T...>& var) {
			ji::match(var, ji::cases{
				[](const auto& error) {
					raise_exception(error);
				}
			});
		}
	public:
		struct promise_type {
			result* m_result = nullptr;

			using ErrorType = ErrorVal;

			result get_return_object() noexcept {
				result res;
				m_result = &res; // guranteed copy elision... i hope
				return res;
			}

			std::suspend_never initial_suspend() const noexcept {
				return {};
			}

			std::suspend_never final_suspend() const noexcept {
				return {};
			}

			void return_value(result&& r) {
				*m_result = std::move(r);
			}

			void unhandled_exception() noexcept {}
		};

	private:
		struct awaiter {
			result m_result;

			bool await_ready() {
				return m_result;
			}

			void await_resume() {}

			template<typename T>
				requires std::constructible_from<ErrorVal, typename T::ErrorType&&>
			void await_suspend(std::coroutine_handle<T> handle) {
				*handle.promise().m_result = ji::error(std::move(m_result).error());
				handle.destroy();
			}
		};

		explicit result(std::coroutine_handle<promise_type> handle)
			: m_coro_handle(handle)
		{}

		std::coroutine_handle<promise_type> m_coro_handle;
	public:

		result(const result& e) {
			m_val = e.m_val;
			if (m_coro_handle) {
				m_coro_handle = e.m_coro_handle;
				m_coro_handle.promise().m_result = this;
			}
		}

		result(result&& e) noexcept {
			m_val = std::move(e.m_val);
			if (m_coro_handle) {
				m_coro_handle = e.m_coro_handle;
				m_coro_handle.promise().m_result = this;
			}
		}

		result& operator= (const result& e) {
			if (&e != this) {
				m_val = e.m_val;
				if (m_coro_handle) {
					m_coro_handle = e.m_coro_handle;
					m_coro_handle.promise().m_result = this;
				}
			}

			return *this;
		}

		result& operator= (result&& e) {
			if (&e != this) {
				m_val = std::move(e.m_val);
				if (m_coro_handle) {
					m_coro_handle = e.m_coro_handle;
					m_coro_handle.promise().m_result = this;
				}
			}

			return *this;
		}

		awaiter operator co_await() const& {
			return awaiter{ *this };
		}

		awaiter operator co_await()&& {
			return awaiter{ std::move(*this) };
		}

		template<typename T>
			requires std::constructible_from<OkVal, const T&>
		result(const ji::error<T>& v) : m_val(v.val) {}

		template<typename T>
			requires std::constructible_from<ErrorVal, T&&>
		result(ji::error<T>&& v) : m_val(std::move(v.val)) {}

		result(ji::ok&& v) : m_val(std::nullopt) {}

		template<typename T>
		result(const ji::error<as_is<T>>& v) : m_val(v.val.val) {}

		template<typename T>
		result(ji::error<as_is<T>>&& v) : m_val(std::move(v.val.val)) {}

		template<typename... Args>
		result(const ji::error<std::variant<Args...>>& v) : m_val(ji::match(v.val, ji::cases{
			[](const ErrorVal& err) { return err; }
		})) { }

		template<typename... Args>
		result(ji::error<std::variant<Args...>>&& v) : m_val(ji::match(std::move(v.val), ji::cases{
			[](ErrorVal&& err) { return std::forward<ErrorVal>(err); }
		})) { }

		result& operator=(const error<ErrorVal>& v) {
			m_val = v.val;
			return *this;
		}

		result& operator=(error<ErrorVal>&& v) {
			m_val = std::move(v.val);
			return *this;
		}

		result& operator=(ok v) {
			m_val = std::nullopt;
			return *this;
		}

		operator bool() const noexcept {
			return !m_val.has_value();
		}

		void check() const {
			if (m_val) {
				raise_exception(m_val.value());
			}
		}

		[[nodiscard]] const ErrorVal& error() const& {
			if (m_val) {
				return m_val.value();
			}

			throw exception::no_error{};
		}

		[[nodiscard]] ErrorVal&& error()&& {
			if (m_val) {
				return std::move(m_val).value();
			}

			throw exception::no_error{};
		}

	private:
		std::optional<ErrorVal> m_val;
	};
}
