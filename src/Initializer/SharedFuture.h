#ifndef SHAREDFUTURE_H
#define SHAREDFUTURE_H

#include <utilites/template_helpers.h>

#include <functional>

namespace initializer {

template<typename T>
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

public:

    template<typename StdSharedFuture>
    SharedFuture(StdSharedFuture sharedFuture) {
        getter = [sharedFuture]() -> T& {
            return *get<T*>(sharedFuture.get());
        };
    }

    T& get() {
        return getter();
    }

private:

    std::function<T&()> getter;
};

} // namespace initializer

#endif // SHAREDFUTURE_H
