
template<typename T>
struct construct_object {
    template<typename... Args>
    static void exec(void* ptr, Args&&... args) {
        new (ptr) T(std::forward<Args>(args)...);
    }
};

template<typename T>
struct construct_object<T&> {
    static void exec(void* ptr, T& init) {
        new (ptr) T*(&init);
    }
};

template<typename T>
struct copy_object {
    static void exec(void* dest, void const* src) {
        new (dest) T(*static_cast<T const*>(src));
    }
};

template<typename T>
struct copy_object<T&> {
    static void exec(void* dest, void const* src) {
        *static_cast<T**>(dest) = *static_cast<T* const*>(src);
    }
};

template<typename T>
struct move_object {
    static void exec(void* dest, void* src) {
        new (dest) T(std::move(*static_cast<T*>(src)));
    }
};

template<typename T>
struct move_object<T&> {
    static void exec(void* dest, void* src) {
        copy_object<T&>::exec(dest, src);
    }
};

template<typename T>
struct destroy_object {
    static void exec(void* p) {
        static_cast<T*>(p)->~T();
    }
};

template<typename T>
struct destroy_object<T&> {
    static void exec(void*) {}
};

