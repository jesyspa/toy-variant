template <typename TStore, typename... Rest>
union variant_data<TStore, Rest...> {
    using representation = typename member_form<TStore>::type;
    // Initialization and destruction must be done externally, as we don't know which element must be created/destroyed.
    variant_data() {}
    ~variant_data() {}

    representation data;
    variant_data<Rest...> next;

    template <typename GetStore>
    void const* cget_impl(std::true_type) const {
        return &data;
    }

    template <typename GetStore>
    void const* cget_impl(std::false_type) const {
        return next.template cget<GetStore>();
    }

    template <typename GetStore>
    void const* cget() const {
        return cget_impl<GetStore>(std::is_same<TStore, GetStore>());
    }

    void const* cget(int n) const {
        return n > 0 ? next.cget(n-1) : &data;
    }

    static destroy_func get_destructor(int n) {
        return n > 0 ? decltype(next)::get_destructor(n-1) : operations<TStore>::destroy;
    }

    static copy_func get_copy_constructor(int n) {
        return n > 0 ? decltype(next)::get_copy_constructor(n-1) : operations<TStore>::copy;
    }

    static move_func get_move_constructor(int n) {
        return n > 0 ? decltype(next)::get_move_constructor(n-1) : operations<TStore>::move;
    }

    static bool is_recursive(int n) {
        return n > 0 ? decltype(next)::is_recursive(n-1) : detail::is_recursive<TStore>::value;
    }
};

template <>
union variant_data<> {
    variant_data(variant_data const&) = delete;
    variant_data& operator=(variant_data const&) = delete;

    template <typename GetStore>
    void const* cget() const {
        internal_error<GetStore>();
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

    static bool is_recursive(int) {
        assert(!"internal error: query of recursivity of non-existent element");
        return false;
    }
};

