#ifndef SHAREDFUTURE_H
#define SHAREDFUTURE_H

#include <utilites/template_helpers.h>

#include <functional>

namespace initializer {

template<typename T0, typename ...T>
class SharedFuture {
private:

    template<typename Arg>
    struct ChooseTagType{
        using type = typename std::conditional<IsTuple<Arg>(), TupleChoiseTag, typename std::conditional<IsPair<Arg>(), PairChoiseTag, OtherChoiseTag>::type>::type;
    };

    template<typename T1, typename Arg>
    static T1 getImpl(OtherChoiseTag, Arg &&arg) {
        return arg;
    }

    template<typename T1, typename ...Args>
    static T1 getImpl(PairChoiseTag, const std::pair<Args...> &arg) {
        return std::get<T1>(arg);
    }

    template<typename T1, typename ...Args>
    static T1 getImpl(TupleChoiseTag, const std::tuple<Args...> &arg) {
        return std::get<T1>(arg);
    }

    template<typename T1, typename Arg>
    static T1 get(Arg &&arg) {
        const typename ChooseTagType<Arg>::type t;
        return getImpl<T1>(t, std::forward<Arg>(arg));
    }

    template<typename StdSharedFuture, typename M>
    static auto makeLambda(StdSharedFuture sharedFuture) {
        return [sharedFuture]() -> M& {
            return *get<M*>(sharedFuture.get());
        };
    }

public:

    template<typename StdSharedFuture>
    SharedFuture(StdSharedFuture sharedFuture) {
        getter = std::make_tuple(
            makeLambda<StdSharedFuture, T0>(sharedFuture),
            makeLambda<StdSharedFuture, T>(sharedFuture)...
        );
    }

    template<bool B = true, typename Dummy = typename std::enable_if<sizeof...(T) == 0 && B>::type>
    T0& get() {
        return std::get<0>(getter)();
    }

    template<typename M>
    M& get() {
        return std::get<std::function<M&()>>(getter)();
    }

private:

    std::tuple<std::function<T0&()>, std::function<T&()>...> getter;
};

} // namespace initializer

#endif // SHAREDFUTURE_H
