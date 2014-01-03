template <typename TStore, typename... Rest>
union variant_data<TStore, Rest...> {
    using representation = typename member_form<TStore>::type;
    using unique_form = typename unique_form<TStore>::type;
    // Initialization and destruction must be done externally, as we don't know which element must be created/destroyed.
    variant_data() {}
    ~variant_data() {}

    representation data;
    variant_data<Rest...> next;

    template <typename GetStore>
    void const* cget() const {
        return cget_impl<GetStore>(std::is_same<TStore, GetStore>());
    }

    void const* cget(int n) const { return n > 0 ? next.cget(n - 1) : &data; }

    template <typename Visitor>
    auto apply(Visitor&& visitor, int n) const -> typename std::remove_reference<Visitor>::type::result_type {
        return n > 0 ? next.apply(std::forward<Visitor>(visitor), n - 1) : visitor(get_ref());
    }

    template <typename Visitor>
    auto apply(Visitor&& visitor, int n) -> typename std::remove_reference<Visitor>::type::result_type {
        return n > 0 ? next.apply(std::forward<Visitor>(visitor), n - 1) : visitor(get_ref());
    }

    static destroy_func get_destructor(int n) {
        return n > 0 ? decltype(next)::get_destructor(n - 1) : operations<TStore>::destroy;
    }

    static copy_func get_copy_constructor(int n) {
        return n > 0 ? decltype(next)::get_copy_constructor(n - 1) : operations<TStore>::copy;
    }

    static move_func get_move_constructor(int n) {
        return n > 0 ? decltype(next)::get_move_constructor(n - 1) : operations<TStore>::move;
    }

    static bool is_recursive(int n) {
        return n > 0 ? decltype(next)::is_recursive(n - 1) : detail::is_recursive<TStore>::value;
    }

private:
    template <typename GetStore>
    void const* cget_impl(std::true_type) const {
        return &data;
    }

    template <typename GetStore>
    void const* cget_impl(std::false_type) const {
        return next.template cget<GetStore>();
    }

    unique_form const& get_cref() const { return operations<TStore>::as_cref(&data); }

    unique_form const& get_ref() const { return get_cref(); }

    unique_form& get_ref() { return const_cast<unique_form&>(get_cref()); }
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

    template <typename Visitor>
    auto apply[[noreturn]](Visitor&&, int) const -> typename std::remove_reference<Visitor>::type::result_type {
        assert(!"internal error: application of visitor to non-existent element");
    }

    template <typename Visitor>
    auto apply[[noreturn]](Visitor&&, int) -> typename std::remove_reference<Visitor>::type::result_type {
        assert(!"internal error: application of visitor to non-existent element");
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

