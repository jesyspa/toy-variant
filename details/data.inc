template <typename T, typename... Rest>
union variant_data<T, Rest...> {
    using normalized = typename normalize<T>::type;
    using representation = typename member_representation<T>::type;
    // Initialization and destruction must be done externally, as we don't know which element must be created/destroyed.
    variant_data() {}
    ~variant_data() {}

    representation data;
    variant_data<Rest...> next;

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

    static destroy_func get_destructor(int n) {
        return n > 0 ? decltype(next)::get_destructor(n-1) : destroy_object<T>::exec;
    }

    static copy_func get_copy_constructor(int n) {
        return n > 0 ? decltype(next)::get_copy_constructor(n-1) : copy_object<T>::exec;
    }

    static move_func get_move_constructor(int n) {
        return n > 0 ? decltype(next)::get_move_constructor(n-1) : move_object<T>::exec;
    }
};

template <>
union variant_data<> {
    variant_data(variant_data const&) = delete;
    variant_data& operator=(variant_data const&) = delete;

    template <typename U>
    void const* cget() const {
        internal_error<U>();
        return nullptr;
    }

    void const* cget(int) const {
        assert(!"internal error: access to non-existent element");
        return nullptr;
    }

    static destroy_func get_destructor(int) {
        assert(!"internal error: access to destructor of non-existent element");
        return nullptr;
    }

    static copy_func get_copy_constructor(int) {
        assert(!"internal error: access to copy-constructor of non-existent element");
        return nullptr;
    }

    static move_func get_move_constructor(int) {
        assert(!"internal error: access to copy-constructor of non-existent element");
        return nullptr;
    }
};

