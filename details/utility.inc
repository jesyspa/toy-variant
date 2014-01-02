template<typename U>
struct internal_error {
    struct not_an_error;
    static_assert(std::is_same<U, not_an_error>::value, "internal error");
};

template<typename, typename>
struct meta_pair {};

template<int N, typename U, typename... Args>
struct pack_find_impl;

template<typename U, typename... Args>
struct pack_find : pack_find_impl<0, typename normalize<U>::type, meta_pair<Args, typename normalize<Args>::type>...> {};

template<int N, typename U, typename T, typename... Args>
struct pack_find_impl<N, U, meta_pair<T, U>, Args...> {
    static constexpr bool found = true;
    static constexpr int value = N;
    using type = T;
};

template<int N, typename U, typename T, typename... Args>
struct pack_find_impl<N, U, T, Args...> : pack_find_impl<N+1, U, Args...> {};

template<int N, typename U>
struct pack_find_impl<N, U> {
    static constexpr bool found = false;
    static constexpr int value = -1;
    using type = U;
};

template<typename U, typename... Ts>
struct pack_contains : std::integral_constant<bool, pack_find<U, Ts...>::found> {};

template<typename U, typename... Ts>
struct assert_pack_contains_helper;

template<typename U, typename T, typename... Ts>
struct assert_pack_contains_helper<U, T, Ts...> : assert_pack_contains_helper<U, Ts...> {};

template<typename U, typename... Ts>
struct assert_pack_contains_helper<U, U, Ts...> : std::true_type {};

template<typename U>
struct assert_pack_contains_helper<U> : std::false_type {};

template<typename U, typename... Ts>
struct assert_pack_contains : assert_pack_contains_helper<U, Ts...> {
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
struct member_representation {
    using type = U;
};

template<typename U>
struct member_representation<U&> {
    using type = U*;
};

template<typename U>
struct normalize :
    std::remove_const<
        typename std::remove_reference<
            typename std::remove_const<U>::type
        >::type> {};

