export module RangeHelpers;

import <ranges>;
import <concepts>;

template <class ContainerType>
concept Container = requires(ContainerType a, const ContainerType b)
{
	requires std::regular<ContainerType>;
	requires std::swappable<ContainerType>;
	requires std::destructible<typename ContainerType::value_type>;
	requires std::same_as<typename ContainerType::reference, typename ContainerType::value_type&>;
	requires std::same_as<typename ContainerType::const_reference, const typename ContainerType::value_type&>;
	requires std::forward_iterator<typename ContainerType::iterator>;
	requires std::forward_iterator<typename ContainerType::const_iterator>;
	requires std::signed_integral<typename ContainerType::difference_type>;
	requires std::same_as<typename ContainerType::difference_type, typename std::iterator_traits<typename ContainerType::iterator>::difference_type>;
	requires std::same_as<typename ContainerType::difference_type, typename std::iterator_traits<typename ContainerType::const_iterator>::difference_type>;
	{ a.begin() } -> std::same_as<typename ContainerType::iterator>;
	{ a.end() } -> std::same_as<typename ContainerType::iterator>;
	{ b.begin() } -> std::same_as<typename ContainerType::const_iterator>;
	{ b.end() } -> std::same_as<typename ContainerType::const_iterator>;
	{ a.cbegin() } -> std::same_as<typename ContainerType::const_iterator>;
	{ a.cend() } -> std::same_as<typename ContainerType::const_iterator>;
	{ a.size() } -> std::same_as<typename ContainerType::size_type>;
	{ a.max_size() } -> std::same_as<typename ContainerType::size_type>;
	{ a.empty() } -> std::same_as<bool>;
};

template <class VectorType>
concept Vector = requires (VectorType a)
{
	requires Container<VectorType>;
	{ a.reserve(1) } -> std::same_as<void>;
};

export namespace ji::ranges {
	template<template<typename> typename T>
	class to {};

	template<template<typename> typename T, std::ranges::range Range, typename R = T<typename decltype(std::begin(std::declval<Range>()))::value_type>>
		requires (Container<R>)
	R operator|(Range&& range, to<T>) {
		R result;

		if constexpr (std::ranges::sized_range<Range> && Vector<R>) {
			result.reserve(range.size());
		}

		std::ranges::copy(std::forward<Range>(range), std::back_inserter(result));

		return result;
	}
}
