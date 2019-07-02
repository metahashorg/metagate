#ifndef MANAGERWRAPPERIMPL_H
#define MANAGERWRAPPERIMPL_H

#include "TypedException.h"

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
constexpr bool IsTuple() {
    return IsTupleImpl<std::decay_t<T>>::value;
}

template<typename F>
constexpr bool IsTupleFunc() {
    return IsTuple<typename std::result_of<F()>::type>();
}

template<typename F>
constexpr bool IsVoidFunc() {
    return std::is_void<typename std::result_of<F()>::type>::value;
}

template<typename F>
constexpr bool IsOneParamFunc() {
    return !IsTupleFunc<F>() && !IsVoidFunc<F>();
}

template<typename F, typename Callback, typename std::enable_if<IsVoidFunc<F>(), int>::type = 0>
void runAndEmitCallback(const F &f, const Callback &callback) {
    const TypedException exception = apiVrapper2(f);
    callback.emitFunc(exception);
}

template<typename F, typename Callback, typename std::enable_if<IsTupleFunc<F>(), int>::type = 0>
void runAndEmitCallback(const F &f, const Callback &callback) {
    typename std::result_of<F()>::type tuple;
    const TypedException exception = apiVrapper2([&tuple, &f](){
        tuple = f();
    });
    apply([&callback](auto&&... args) {
        callback.emitFunc(std::forward<decltype(args)>(args)...);
    }, std::tuple_cat(std::make_tuple(exception), tuple));
}

template<typename F, typename Callback, typename std::enable_if<IsOneParamFunc<F>(), int>::type = 0>
void runAndEmitCallback(const F &f, const Callback &callback) {
    typename std::result_of<F()>::type param;
    const TypedException exception = apiVrapper2([&param, &f](){
        param = f();
    });
    callback.emitFunc(exception, param);
}

#endif // MANAGERWRAPPERIMPL_H
