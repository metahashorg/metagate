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

template<int> struct ChoiseRunAndEmitCallback {};
using VoidChoiseRunAndEmitCallback = ChoiseRunAndEmitCallback<0>;
using TupleChoiseRunAndEmitCallback = ChoiseRunAndEmitCallback<1>;
using OtherChoiseRunAndEmitCallback = ChoiseRunAndEmitCallback<2>;

template<typename F, typename Callback>
void runAndEmitCallbackImpl(VoidChoiseRunAndEmitCallback, const F &f, const Callback &callback) {
    const TypedException exception = apiVrapper2(f);
    callback.emitFunc(exception);
}

template<typename F, typename Callback>
void runAndEmitCallbackImpl(TupleChoiseRunAndEmitCallback, const F &f, const Callback &callback, const typename std::result_of<F()>::type& defaultParams = typename std::result_of<F()>::type{}) {
    typename std::result_of<F()>::type tuple(defaultParams);
    const TypedException exception = apiVrapper2([&tuple, &f](){
        tuple = f();
    });
    apply([&callback](auto&&... args) {
        callback.emitFunc(std::forward<decltype(args)>(args)...);
    }, std::tuple_cat(std::make_tuple(exception), tuple));
}

template<typename F, typename Callback>
void runAndEmitCallbackImpl(OtherChoiseRunAndEmitCallback, const F &f, const Callback &callback, const std::tuple<typename std::result_of<F()>::type>& defaultParams = std::tuple<typename std::result_of<F()>::type>{}) {
    typename std::result_of<F()>::type param(std::get<0>(defaultParams));
    const TypedException exception = apiVrapper2([&param, &f](){
        param = f();
    });
    callback.emitFunc(exception, param);
}

template<typename F>
struct RunAndEmitTagType{
    using type = typename std::conditional<IsVoidFunc<F>(), VoidChoiseRunAndEmitCallback, typename std::conditional<IsTupleFunc<F>(), TupleChoiseRunAndEmitCallback, OtherChoiseRunAndEmitCallback>::type>::type;
};

template<typename F, typename Callback, typename Val, typename... Vals>
void runAndEmitCallback(const F &f, const Callback &callback, Val&& defaultVal1, Vals&&... defaultVals) {
    const typename RunAndEmitTagType<F>::type tag;
    runAndEmitCallbackImpl(tag, f, callback, std::tuple_cat(std::make_tuple(std::forward<Val>(defaultVal1)), std::make_tuple(std::forward<Vals>(defaultVals)...)));
}

template<typename F, typename Callback>
void runAndEmitCallback(const F &f, const Callback &callback) {
    const typename RunAndEmitTagType<F>::type tag;
    runAndEmitCallbackImpl(tag, f, callback);
}

template<typename F, typename Callback>
void runAndEmitErrorCallback(const F &f, const Callback &callback) {
    static_assert(std::is_void<decltype(f())>::value, "Not void");
    const TypedException exception = apiVrapper2(f);
    if (exception.isSet()) {
        callback.emitException(exception);
    }
}

#endif // MANAGERWRAPPERIMPL_H
