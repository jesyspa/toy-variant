template <typename T, typename... Rest>
union variant_data_impl<T, Rest...> {
    using normalized = typename parameter_normalize<T>::type;
    // Initialization and destruction must be done externally, as we don't know which element must be created/destroyed.
    variant_data_impl() {}
    ~variant_data_impl() {}

    T data;
    variant_data_impl<Rest...> next;

    template <typename U>
    void const* cget_impl(std::true_type) const {
        return &data;
    }

    template <typename U>
    void const* cget_impl(std::false_type) const {
        return next.template cget<U>();
    }

    template <typename U>
    void const* cget() const {
        return cget_impl<U>(std::is_same<normalized, U>());
    }

    void const* cget(int n) const {
        return n > 0 ? next.cget(n-1) : &data;
    }

    destroy_func get_destructor(int n) const {
        return n > 0 ? next.get_destructor(n-1) : [](void* p) { static_cast<T*>(p)->~T(); };
    }

    copy_func get_copy_constructor(int n) const {
        return n > 0 ? next.get_copy_constructor(n-1) : [](void* p, void const* other) { new (p) T(*static_cast<T const*>(other)); };
    }

    move_func get_move_constructor(int n) const {
        return n > 0 ? next.get_move_constructor(n-1) : [](void* p, void* other) { new (p) T(std::move(*static_cast<T*>(other))); };
    }
};

inline void destroy_ptr(void*) {}

template<typename T>
inline void copy_ptr(void* dest, void const* src) {
    *static_cast<T**>(dest) = *static_cast<T* const*>(src);
}

template<typename T>
inline void move_ptr(void* dest, void const* src) {
    *static_cast<T**>(dest) = *static_cast<T**>(src);
}

template <typename T, typename... Rest>
union variant_data_impl<T&, Rest...> {
    using normalized = typename parameter_normalize<T>::type;
    variant_data_impl() {}
    ~variant_data_impl() {}

    T* data;
    variant_data_impl<Rest...> next;

    template <typename U>
    void const* cget_impl(std::true_type) const {
        return &data;
    }

    template <typename U>
    void const* cget_impl(std::false_type) const {
        return next.template cget<U>();
    }

    template <typename U>
    void const* cget() const {
        return cget_impl<U>(std::is_same<normalized, U>());
    }

    void const* cget(int n) const {
        return n > 0 ? next.cget(n-1) : &data;
    }

    destroy_func get_destructor(int n) const {
        return n > 0 ? next.get_destructor(n-1) : destroy_ptr;
    }

    copy_func get_copy_constructor(int n) const {
        return n > 0 ? next.get_copy_constructor(n-1) : copy_ptr<T>;
    }

    move_func get_move_constructor(int n) const {
        return n > 0 ? next.get_move_constructor(n-1) : move_ptr<T>;
    }
};


template <>
union variant_data_impl<> {
    variant_data_impl(variant_data_impl const&) = delete;
    variant_data_impl& operator=(variant_data_impl const&) = delete;

    template <typename U>
    void const* cget() const {
        internal_error<U>();
        return nullptr;
    }

    void const* cget(int) const {
        assert(!"internal error: access to non-existent element");
        return nullptr;
    }

    destroy_func get_destructor(int) const {
        assert(!"internal error: access to destructor of non-existent element");
        return nullptr;
    }

    copy_func get_copy_constructor(int) const {
        assert(!"internal error: access to copy-constructor of non-existent element");
        return nullptr;
    }

    move_func get_move_constructor(int) const {
        assert(!"internal error: access to copy-constructor of non-existent element");
        return nullptr;
    }
};

template<typename... Types>
struct variant_data {
    variant_data_impl<Types...> data;

    template <typename U>
    void const* get() const {
        return data.template cget<U>();
    }

    template <typename U>
    void* get() {
        return const_cast<void*>(data.template cget<U>());
    }

    void const* get(int n) const {
        return data.cget(n);
    }

    void* get(int n) {
        return const_cast<void*>(data.cget(n));
    }

    destroy_func get_destructor(int n) const {
        return data.get_destructor(n);
    }

    copy_func get_copy_constructor(int n) const {
        return data.get_copy_constructor(n);
    }

    move_func get_move_constructor(int n) const {
        return data.get_move_constructor(n);
    }
};
