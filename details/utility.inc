template<typename U>
struct internal_error {
    struct not_an_error;
    static_assert(std::is_same<U, not_an_error>::value, "internal error");
};

template<int N, typename U, typename... Args>
struct pack_find_impl;

template<typename U, typename... Args>
struct pack_find : pack_find_impl<0, U, Args...> {};

template<int N, typename U, typename... Args>
struct pack_find_impl<N, U, U, Args...> {
    static constexpr bool found = true;
    static constexpr int value = N;
};

template<int N, typename U, typename T, typename... Args>
struct pack_find_impl<N, U, T, Args...> : pack_find_impl<N+1, U, Args...> {};

template<int N, typename U>
struct pack_find_impl<N, U> {
    static constexpr bool found = false;
    static constexpr int value = -1;
};

template<typename U, typename... Ts>
struct pack_contains : std::integral_constant<bool, pack_find<U, Ts...>::found> {};

template<typename U, typename... Ts>
struct assert_pack_contains : pack_contains<U, Ts...> {
    static_assert(assert_pack_contains::value, "variant has no such member");
};

template<typename... Ts>
struct has_duplicates;

template<>
struct has_duplicates<> : std::false_type {};

template<typename U, typename... Ts>
struct has_duplicates<U, Ts...> :
    std::integral_constant<bool, pack_contains<U, Ts...>::value || has_duplicates<Ts...>::value> {};

template<typename U>
struct member_normalize {
    using type = U;
};

template<typename U>
struct parameter_normalize : 
    std::remove_const<
        typename std::remove_reference<
            typename std::remove_const<U>::type
        >::type> {};

template<typename T>
struct construct {
    template<typename... Args>
    static void exec(void* ptr, Args&&... args) {
        new (ptr) T(std::forward<Args>(args)...);
    }
};

template<typename T>
struct construct<T&> {
    static void exec(void* ptr, T& init) {
        new (ptr) T*(&init);
    }
};
