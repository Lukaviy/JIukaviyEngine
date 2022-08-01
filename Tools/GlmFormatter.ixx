module;

#include <glm/glm.hpp>

export module GlmFormatter;

import <format>;

export namespace std {
	template<class T, glm::length_t Length, glm::qualifier Qualifier, class CharT>
		struct formatter<glm::vec<Length, T, Qualifier>, CharT> : formatter<T, CharT> {
		template<class FormatContext>
		auto format(glm::vec<Length, T, Qualifier> vec, FormatContext& fc) const {
			format_to(fc.out(), "[");
			for (glm::length_t i = 0; i < Length; ++i) {
				formatter<T, CharT>::format(vec[i], fc);
				if (i < Length - 1) {
					format_to(fc.out(), ", ");
				}
			}
			return format_to(fc.out(), "]");
		}
	};
}
