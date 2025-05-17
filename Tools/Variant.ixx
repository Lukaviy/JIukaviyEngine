export module Variant;

export import <variant>;

export namespace ji {
	template<class... Ts> struct cases : Ts... { using Ts::operator()...; };
	template<class... Ts> cases(Ts...)->cases<Ts...>;

	template<typename V, typename Callable>
	decltype(auto) match(V&& v, Callable&& c) {
#ifdef IOS
		if (!v.valueless_by_exception()) {
			return std::__variant_detail::__visitation::__variant::__visit_value(std::forward<Callable>(c), std::forward<V>(v));
		}
		else {
			throw std::runtime_error("valueless variant");
		}
#else
		return std::visit(std::forward<Callable>(c), std::forward<V>(v));
#endif
	}
}
