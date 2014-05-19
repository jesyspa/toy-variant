template <typename T>
struct internal_error {
    struct not_an_error;
    static_assert(std::is_same<T, not_an_error>::value, "internal error");
};

template <typename, typename>
struct meta_pair {};

template <int N, typename T, typename... Pack>
struct pack_find_impl;

template <template <typename> class NORM, typename T, typename... Pack>
struct pack_find : pack_find_impl<0, typename NORM<T>::type, meta_pair<Pack, typename NORM<Pack>::type>...> {};

template <int N, typename T, typename P, typename... Pack>
struct pack_find_impl<N, T, meta_pair<P, T>, Pack...> {
    static constexpr bool found = true;
    static constexpr int value = N;
    using type = P;
};

template <int N, typename T, typename P, typename... Pack>
struct pack_find_impl<N, T, P, Pack...> : pack_find_impl<N + 1, T, Pack...> {};

template <int N, typename T>
struct pack_find_impl<N, T> {
    static constexpr bool found = false;
    static constexpr int value = -1;
    using type = T;
};

template <template <typename> class NORM, typename T, typename... Pack>
struct pack_contains : std::integral_constant<bool, pack_find<NORM, T, Pack...>::found> {};

template <typename T, typename... Pack>
struct assert_pack_contains_helper;

template <typename T, typename P, typename... Pack>
struct assert_pack_contains_helper<T, P, Pack...> : assert_pack_contains_helper<T, Pack...> {};

template <typename T, typename... Pack>
struct assert_pack_contains_helper<T, T, Pack...> : std::true_type {};

template <typename T>
struct assert_pack_contains_helper<T> : std::false_type {};

template <template <typename> class NORM, typename T, typename... Pack>
struct assert_pack_contains : pack_contains<NORM, T, Pack...> {
    static_assert(assert_pack_contains::value, "variant has no such member");
};

template <template <typename> class NORM, typename... Pack>
struct has_duplicates;

template <template <typename> class NORM>
struct has_duplicates<NORM> : std::false_type {};

template <template <typename> class NORM, typename T, typename... Pack>
struct has_duplicates<NORM, T, Pack...> : std::integral_constant<bool, pack_contains<NORM, T, Pack...>::value ||
                                                                           has_duplicates<NORM, Pack...>::value> {};

namespace impl {
template <typename T>
struct remove_recursive_helper {
    using type = T;
};

template <typename T>
struct remove_recursive_helper<recursive_helper<T>> {
    using type = T;
};

template <typename T>
struct make_member_form {
    using type = T;
};

template <typename T>
struct make_member_form<T&> {
    using type = T*;
};

template <typename T>
struct make_member_form<recursive_helper<T>> {
    using type = T*;
};
}

template <typename T>
struct query_form : impl::remove_recursive_helper<T> {};

template <typename T>
struct store_form : std::remove_const<T> {};

template <typename T>
struct store_form<T&> : std::add_lvalue_reference<typename store_form<T>::type> {};

template <typename T>
struct store_form<recursive_helper<T>> {
    using type = recursive_helper<typename store_form<T>::type>;
};

template <typename T>
struct unique_form : std::remove_reference<typename store_form<typename query_form<T>::type>::type> {};

template <typename T>
struct member_form : impl::make_member_form<typename store_form<T>::type> {};

