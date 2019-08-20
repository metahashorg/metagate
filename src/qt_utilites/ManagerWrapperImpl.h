#ifndef MANAGERWRAPPERIMPL_H
#define MANAGERWRAPPERIMPL_H

#include "TypedException.h"

#include "utilites/template_helpers.h"

template<typename F, typename Callback>
void runAndEmitCallbackImpl(VoidChoiseTag, const F &f, const Callback &callback) {
    const TypedException exception = apiVrapper2(f);
    callback.emitFunc(exception);
}

template<typename F, typename Callback>
void runAndEmitCallbackImpl(TupleChoiseTag, const F &f, const Callback &callback, const typename std::result_of<F()>::type& defaultParams = typename std::result_of<F()>::type{}) {
    typename std::result_of<F()>::type tuple(defaultParams);
    const TypedException exception = apiVrapper2([&tuple, &f](){
        tuple = f();
    });
    apply([&callback](auto&&... args) {
        callback.emitFunc(std::forward<decltype(args)>(args)...);
    }, std::tuple_cat(std::make_tuple(exception), tuple));
}

template<typename F, typename Callback>
void runAndEmitCallbackImpl(OtherChoiseTag, const F &f, const Callback &callback, const std::tuple<typename std::result_of<F()>::type>& defaultParams = std::tuple<typename std::result_of<F()>::type>{}) {
    typename std::result_of<F()>::type param(std::get<0>(defaultParams));
    const TypedException exception = apiVrapper2([&param, &f](){
        param = f();
    });
    callback.emitFunc(exception, param);
}

template<typename F>
struct RunAndEmitTagType{
    using type = typename std::conditional<IsVoidFunc<F>(), VoidChoiseTag, typename std::conditional<IsTupleFunc<F>(), TupleChoiseTag, OtherChoiseTag>::type>::type;
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
