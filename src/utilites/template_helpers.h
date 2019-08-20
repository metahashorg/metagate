#ifndef TEMPLATE_HELPERS_H
#define TEMPLATE_HELPERS_H

#include <tuple>

namespace detail {
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}
}  // namespace detail

template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t) {
    return detail::apply_impl(
        std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

template <typename T>
struct IsTupleImpl : std::false_type {};

template <typename... U>
struct IsTupleImpl<std::tuple <U...>> : std::true_type {};

template <typename T>
struct IsPairImpl : std::false_type {};

template <typename... U>
struct IsPairImpl<std::pair <U...>> : std::true_type {};

template <typename T>
constexpr bool IsTuple() {
    return IsTupleImpl<std::decay_t<T>>::value;
}

template <typename T>
constexpr bool IsPair() {
    return IsPairImpl<std::decay_t<T>>::value;
}

template<typename F>
constexpr bool IsTupleFunc() {
    return IsTuple<typename std::result_of<F()>::type>();
}

template<typename F>
constexpr bool IsPairFunc() {
    return IsPair<typename std::result_of<F()>::type>();
}

template<typename F>
constexpr bool IsVoidFunc() {
    return std::is_void<typename std::result_of<F()>::type>::value;
}

template<int> struct ChoiseTag {};
using VoidChoiseTag = ChoiseTag<0>;
using TupleChoiseTag = ChoiseTag<1>;
using PairChoiseTag = ChoiseTag<2>;
using OtherChoiseTag = ChoiseTag<3>;

#endif // TEMPLATE_HELPERS_H
