export module variant;

export namespace ji {
	template<class... Ts> struct cases : Ts... { using Ts::operator()...; };
	template<class... Ts> cases(Ts...)->cases<Ts...>;
}
