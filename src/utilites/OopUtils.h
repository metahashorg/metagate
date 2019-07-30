#ifndef OOP_UTILS_H_
#define OOP_UTILS_H_

class no_copyable {
public:
    
    no_copyable() = default;
    ~no_copyable() = default;
    
    no_copyable(const no_copyable&) = delete;
    no_copyable& operator=(const no_copyable&) = delete;
    
    no_copyable(no_copyable&&) = default;
    no_copyable& operator=(no_copyable&&) = default;
    
};

class no_moveable {
public:
    
    no_moveable() = default;
    ~no_moveable() = default;
    
    no_moveable(no_moveable&&) = delete;
    no_moveable& operator=(no_moveable&&) = delete;
    
    no_moveable(const no_moveable&) = default;
    no_moveable& operator=(const no_moveable&) = default;
    
};

// Обертка для деструктора shared_ptr (для unique_ptr деструктор проверяет объект на null перед удалением)
#define DELETER(function) \
    [](auto *element) { \
        if (element != nullptr) { \
            function(element); \
        } \
    }
    
#endif // OOP_UTILS_H_
