module;

#include <compare>

export module KeyId;

export namespace ji {
	template<typename T, typename S = size_t>
	struct KeyId {
	public:
		KeyId(S&& id) : m_id(std::move(id)) {}

		explicit operator T() const {
			return m_id;
		}

		auto operator<=>(const KeyId&) const = default;
	private:
		S m_id;
	};
}
