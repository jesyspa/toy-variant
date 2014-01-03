template<typename S>
struct operations {
    template<typename... Args>
    static void construct(void* ptr, Args&&... args) {
        new (ptr) S(std::forward<Args>(args)...);
    }

    static void copy(void* dest, void const* src) {
        new (dest) S(*static_cast<S const*>(src));
    }

    static void move(void* dest, void* src) {
        new (dest) S(std::move(*static_cast<S*>(src)));
        destroy(src);
    }

    static void destroy(void* p) {
        static_cast<S*>(p)->~S();
    }

    static S const& as_cref(void const* p) {
        return *static_cast<S const*>(p);
    }
};

template<typename S>
struct operations<S&> {
    static void construct(void* ptr, S& init) {
        new (ptr) S*(&init);
    }

    static void copy(void* dest, void const* src) {
        *static_cast<S**>(dest) = *static_cast<S* const*>(src);
    }

    static void move(void* dest, void* src) {
        copy(dest, src);
    }

    static void destroy(void*) {}

    static S const& as_cref(void const* p) {
        return **static_cast<S* const*>(p);
    }
};

template<typename S>
struct operations<recursive_helper<S>> {
    template<typename... Args>
    static void construct(void* ptr, Args&&... args) {
        *static_cast<S**>(ptr) = new S(std::forward<Args>(args)...);
    }

    static void copy(void* dest, void const* src) {
        *static_cast<S**>(dest) = new S(**static_cast<S* const*>(src));
    }

    static void move(void* dest, void* src) {
        *static_cast<S**>(dest) = *static_cast<S**>(src);
        *static_cast<S**>(src) = nullptr;
    }

    static void destroy(void* p) {
        delete *static_cast<S**>(p);
    }

    static S const& as_cref(void const* p) {
        return **static_cast<S* const*>(p);
    }
};
