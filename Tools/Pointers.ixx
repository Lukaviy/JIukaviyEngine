module;

#include <memory>
#include <concepts>
#include <type_traits>

export module Pointers;

export namespace ji {
	template<typename Object, typename NotNullObject, typename NotOwnerNotNullObject>
	struct crtp_pointer_opt {
		[[nodiscard]] NotOwnerNotNullObject value() & {
			auto ptr = static_cast<const Object*>(this)->get_unsafe_raw();
			if (!ptr) {
				throw std::exception("null optional");
			}
			return { ptr };
		}

		[[nodiscard]] NotNullObject&& value() && {
			if (!static_cast<const Object*>(this)->get_unsafe_raw()) {
				throw std::exception("null optional");
			}
			return std::move(*static_cast<NotNullObject*>(static_cast<Object*>(this)));
		}

		[[nodiscard]] constexpr bool has_value() const noexcept {
			return static_cast<const Object*>(this)->get_unsafe_raw() != nullptr;
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept {
			return has_value();
		}

		template<typename U>
			requires std::constructible_from<NotNullObject, U&&>
		[[nodiscard]] constexpr NotNullObject value_or(U&& default_value) const& {
			if (!static_cast<const Object*>(this)->get_unsafe_raw()) {
				return { std::forward<U>(default_value) };
			}
			return *static_cast<const NotNullObject*>(this);
		}

		template<typename U>
		[[nodiscard]] constexpr NotNullObject value_or(U&& default_value)&& {
			if (!static_cast<const Object*>(this)->get_unsafe_raw()) {
				return static_cast<NotNullObject>(std::forward<U>(default_value));
			}
			return std::move(*static_cast<const NotNullObject*>(this));
		}
	};

	template<typename T>
	concept PointerValueType = !std::is_reference_v<T>;

	template<PointerValueType T>
	class ref {
	public:
		using value_type = T;

	public:
		ref(T& reference) noexcept {
			m_pointer = &reference;
		}

		template<typename U>
			requires std::is_convertible_v<std::remove_reference_t<U>*, T*>
		ref(U& reference) noexcept {
			m_pointer = static_cast<value_type*>(&reference);
		}

		ref(const ref& reference) noexcept : ref(reference.get_unsafe_raw()) {}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		ref(const ref<U>& reference) noexcept : ref(*static_cast<T*>(reference.get_unsafe_raw())) {}

		ref& operator=(ref other) noexcept {
			m_pointer = other.m_pointer;
			return *this;
		}

		T* operator->() const noexcept {
			return get_unsafe_raw();
		}

		T& operator*() noexcept {
			return *get_unsafe_raw();
		}

		[[nodiscard]] explicit operator bool() const noexcept = delete;

		T* get_unsafe_raw() const noexcept {
			return m_pointer;
		}

		operator T* () const noexcept {
			return get_unsafe_raw();
		}

		operator T& () const noexcept {
			return *get_unsafe_raw();
		}

		template<typename U>
			requires std::is_convertible_v<T*, U*>
		auto operator<=>(U* ptr) const noexcept {
			return m_pointer <=> ptr;
		}

		template<typename U>
			requires std::is_convertible_v<T*, U*>
		auto operator<=>(ref<U> ptr) const noexcept {
			return m_pointer <=> ptr.get_unsafe_raw();
		}

		bool operator==(std::nullptr_t) = delete;
		bool operator!=(std::nullptr_t) = delete;

	protected:
		constexpr ref() noexcept = default;
		constexpr ref(std::nullptr_t) noexcept : ref() {}

	protected:
		T* m_pointer;
	};

	template<typename T> ref(T&&)->ref<std::remove_reference_t<T>>;
	template<typename T> ref(T*)->ref<T>;

	template<typename T>
	bool operator==(std::nullptr_t, const ref<T>&) = delete;

	template<typename T>
	bool operator!=(std::nullptr_t, const ref<T>&) = delete;

	template<PointerValueType T>
	class ref_opt : protected ref<T>, public crtp_pointer_opt<ref_opt<T>, ref<T>, ref<T>> {
	public:
		using value_type = typename ref<T>::value_type;

		template<class, class, class>
		friend struct crtp_pointer_opt;

		using ref<T>::ref;

	public:
		constexpr ref_opt() noexcept : ref<T>() {
			ref<T>::m_pointer = nullptr;
		}

		constexpr ref_opt(std::nullptr_t) noexcept : ref_opt() {}

		ref_opt(T* ptr) noexcept {
			ref<T>::m_pointer = ptr;
		}

		ref_opt(const ref_opt& reference) noexcept {
			ref<T>::m_pointer = reference.m_pointer;
		}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		ref_opt(const ref_opt<U>& reference) noexcept {
			ref<T>::m_pointer = static_cast<T*>(reference.get_unsafe_raw());
		}

		ref_opt(const ref<T>& reference) noexcept : ref_opt(reference.get_unsafe_raw()) {}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		ref_opt(const ref<U>& reference) noexcept : ref_opt(static_cast<value_type*>(reference.get_unsafe_raw())) {}

		ref_opt& operator=(ref_opt other) {
			ref<T>::m_pointer = other.m_pointer;
			return *this;
		}

		[[nodiscard]] explicit operator bool() const noexcept {
			return get_unsafe_raw();
		}

		using ref<T>::get_unsafe_raw;
		using ref<T>::operator<=>;

		template<typename U>
			requires std::is_convertible_v<T*, U*>
		auto operator<=>(ref<U> ptr) const noexcept {
			return get_unsafe_raw() <=> ptr.get_unsafe_raw();
		}

		template<typename U>
			requires std::is_convertible_v<T*, U*>
		auto operator<=>(ref_opt<U> ptr) const noexcept {
			return get_unsafe_raw() <=> ptr.get_unsafe_raw();
		}

		bool operator==(std::nullptr_t) const noexcept {
			return !get_unsafe_raw();
		}

		bool operator!=(std::nullptr_t) const noexcept {
			return get_unsafe_raw();
		}

		T* operator->() const noexcept = delete;

		T& operator*() noexcept = delete;

		using ref<T>::operator T*;

		operator T& () const noexcept = delete;
	};

	template<typename T> 
	ref_opt(T&&)->ref_opt<std::remove_reference_t<T>>;

	template<typename T> 
	ref_opt(T*)->ref_opt<T>;

	template<PointerValueType T>
	class unique : protected std::unique_ptr<T> {
	public:
		using value_type = T;
		using pointer = value_type*;

		template<PointerValueType U>
		friend class unique;

		template<PointerValueType U>
		friend class unique_opt;
	public:
		explicit unique(T* ptr) noexcept : std::unique_ptr<T>(ptr) {}

		unique(const unique&) = delete;

		unique(unique&& ptr) noexcept : std::unique_ptr<T>(std::move(ptr).release()) {}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique(unique<U>&& ptr) noexcept : std::unique_ptr<T>(std::move(ptr).release()) {}

		[[nodiscard]] ji::ref<T> release() && noexcept {
			return *std::unique_ptr<T>::release();
		}

		unique& operator=(unique&& ptr) noexcept {
			std::unique_ptr<T>::operator=(std::unique_ptr<T>(std::move(ptr).release()));
			return *this;
		}

		unique& operator=(unique&) = delete;

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique<T>& operator=(unique<U>&& ptr) noexcept {
			std::unique_ptr<T>::operator=(std::unique_ptr<T>(std::move(ptr).release()));
			return *this;
		}

		[[nodiscard]] ji::ref<T> ref() const& noexcept {
			return ji::ref<T>(*std::unique_ptr<T>::get());
		}

		ji::ref<T> ref() && noexcept = delete;

		[[nodiscard]] T* get_unsafe_raw() const noexcept {
			return std::unique_ptr<T>::get();
		}

		[[nodiscard]] T* operator->() const noexcept {
			return get_unsafe_raw();
		}

		[[nodiscard]] T& operator*() const noexcept {
			return *get_unsafe_raw();
		}

		[[nodiscard]] auto operator<=>(const unique& ptr) {
			return get_unsafe_raw() <=> ptr.get_unsafe_raw();
		}

		template<typename U>
			requires std::is_convertible_v<T*, U*>
		[[nodiscard]] auto operator<=>(U* ptr) const noexcept {
			return get_unsafe_raw() <=> ptr;
		}

		bool operator==(std::nullptr_t) = delete;
		bool operator!=(std::nullptr_t) = delete;

		operator bool() const noexcept = delete;
	protected:
		void reset(pointer ptr = pointer()) noexcept {
			std::unique_ptr<T>::reset(ptr);
		}

		[[nodiscard]] ji::ref<T> release() & noexcept {
			return *std::unique_ptr<T>::release();
		}

		unique make_potential_null(T* ptr) {
			unique res;
			static_cast<std::unique_ptr<T>&>(res) = ptr;
			return res;
		}

		unique make_potential_null(std::unique_ptr<T> ptr) {
			unique res;
			static_cast<std::unique_ptr<T>&>(res) = std::move(ptr);
			return res;
		}

		constexpr unique() : std::unique_ptr<T>(nullptr) {}
		constexpr unique(std::nullptr_t) : unique() {}
	};

	template<typename T>
	bool operator==(std::nullptr_t, const unique<T>&) = delete;

	template<typename T>
	bool operator!=(std::nullptr_t, const unique<T>&) = delete;

	template<typename T, typename ...Args>
		requires std::constructible_from<T, Args...>
	unique<T> make_unique(Args&&... args) {
		auto sptr = std::make_unique<T>(std::forward<Args>(args)...);
		return unique<T>(sptr.release());
	}

	template<PointerValueType T>
	class unique_opt : protected unique<T>, public crtp_pointer_opt<unique_opt<T>, unique<T>, ref<T>> {
		template<class, class, class> friend struct crtp_pointer_opt;
	public:
		using value_type = T;
		using pointer = value_type*;

	public:
		unique_opt() noexcept : unique<T>() {}
		unique_opt(std::nullptr_t) noexcept : unique<T>() {}

		explicit unique_opt(T* ptr) noexcept : unique<T>(make_potential_null(ptr)) {}

		unique_opt(std::unique_ptr<T>&& ptr) noexcept : unique<T>(make_potential_null(std::move(ptr))) {}

		unique_opt(unique<T>&& ptr) noexcept : unique<T>(std::move(ptr)) {}

		unique_opt(unique_opt&& ptr) noexcept = default;

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique_opt(unique_opt<U>&& ptr) noexcept : unique<T>(std::move(ptr).release().get_unsafe_raw()) {}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique_opt(unique<U>&& ptr) noexcept : unique<T>(std::move(ptr).release().get_unsafe_raw()) {}

		unique_opt& operator=(std::nullptr_t) noexcept {
			unique<T>::reset();
			return *this;
		}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique_opt& operator=(std::unique_ptr<U>&& ptr) noexcept {
			unique<T>::operator=(make_potential_null(std::move(ptr)));
			return *this;
		}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique_opt& operator=(unique<U>&& ptr) noexcept {
			unique<T>::operator=(unique<T>(std::move(ptr)));
			return *this;
		}

		unique_opt& operator=(unique_opt&& ptr) noexcept {
			unique<T>::operator=(make_potential_null(std::move(ptr).release().get_unsafe_raw()));
			return *this;
		}

		template<typename U>
			requires std::is_convertible_v<U*, T*>
		unique_opt& operator=(unique_opt<U>&& ptr) noexcept {
			unique<T>::operator=(make_potential_null(std::move(ptr).release().get_unsafe_raw()));
			return *this;
		}

		operator bool() const noexcept {
			return get_unsafe_raw();
		}

		template<typename U>
			requires std::is_convertible_v<T*, U*> || std::is_convertible_v<U*, T*>
		[[nodiscard]] auto operator<=>(U* ptr) const noexcept {
			return get_unsafe_raw() <=> ptr;
		}

		[[nodiscard]] auto operator<=>(const unique<T>& ptr) {
			return get_unsafe_raw() <=> ptr.get_unsafe_raw();
		}

		[[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
			return get_unsafe_raw() == nullptr;
		}

		[[nodiscard]] bool operator!=(std::nullptr_t) const noexcept {
			return get_unsafe_raw() != nullptr;
		}

		[[nodiscard]] ref_opt<T> ref() const noexcept {
			return get_unsafe_raw();
		}

		[[nodiscard]] ref_opt<T> release() noexcept {
			return ref_opt<T>(std::unique_ptr<T>::release());
		}

		const unique<T>& operator*() noexcept = delete;
		const unique<T>* operator->() noexcept = delete;

		void reset(pointer ptr = pointer()) noexcept {
			unique<T>::reset(ptr);
		}

		using unique<T>::get_unsafe_raw;
	};

	template<PointerValueType T>
	unique_opt(T*)->unique_opt<T>;
}