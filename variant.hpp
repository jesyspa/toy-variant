#pragma once

#include <cassert>
#include <type_traits>

#include <boost/mpl/or.hpp>

namespace detail {
using destroy_func = void(*)(void*);
using copy_func = void(*)(void*, void const*);
using move_func = void(*)(void*, void*);

template<typename U>
struct internal_error;

template<typename...>
union variant_data_impl;

template<typename...>
struct variant_data;

template<typename U, typename... Ts>
using pack_contains = boost::mpl::or_<std::is_same<U, Ts>...>;

template<typename U, typename... Ts>
struct pack_find;

template<typename U, typename... Ts>
struct assert_pack_contains;

template<typename U>
struct member_normalize;

template<typename U>
struct parameter_normalize;

template<typename T>
struct construct;
}

template<typename... Types>
class variant {
    template<typename U>
    using member_normalize = typename detail::member_normalize<U>::type;
    template<typename U>
    using parameter_normalize = typename detail::parameter_normalize<U>::type;
    template<typename U>
    using assert_pack_contains = detail::assert_pack_contains<parameter_normalize<U>, parameter_normalize<Types>...>;
    template<typename U>
    using assert_pack_explicitly_contains = detail::assert_pack_contains<U, Types...>;
    template<typename U>
    using pack_find = detail::pack_find<parameter_normalize<U>, parameter_normalize<Types>...>;

    detail::variant_data<member_normalize<Types>...> data;
    int current = sizeof...(Types);

    template<typename U>
    void* get() {
        return data.template get<parameter_normalize<U>>();
    }

    template<typename U>
    void const* get() const {
        return data.template get<parameter_normalize<U>>();
    }

    template<typename U>
    U const& get_cref_impl(std::true_type) const {
        using V = typename std::remove_reference<U>::type;
        return **static_cast<V* const*>(get<U>());
    }

    template<typename U>
    U const& get_cref_impl(std::false_type) const {
        return *static_cast<U const*>(get<U>());
    }

    template<typename U>
    U const& get_cref() const {
        return get_cref_impl<U>(std::is_reference<U>{});
    }

    template<typename U>
    U& get_ref() {
        return const_cast<U&>(get_cref<U>());
    }

    void destroy_current() {
        if (current == sizeof...(Types))
            return;
        auto p = data.get(current);
        auto f = data.get_destructor(current);
        f(p);
        current = sizeof...(Types);
    }

    void copy(variant const& v) {
        if (v.current == sizeof...(Types))
            return;
        copy(v.current, v.data.get(v.current));
        current = v.current;
    }

    void move(variant&& v) {
        if (v.current == sizeof...(Types))
            return;
        move(v.current, v.data.get(v.current));
        current = v.current;
    }

    void copy(int n, void const* other) {
        auto f = data.get_copy_constructor(n);
        auto p = data.get(n);
        f(p, other);
    }

    void move(int n, void* other) {
        auto f = data.get_move_constructor(n);
        auto p = data.get(n);
        f(p, other);
    }

public:
    variant() = default;

    variant(variant const& v) {
        copy(v);
    }

    variant(variant&& v) {
        move(std::move(v));
    }

    template<typename U, typename = std::enable_if_t<pack_find<U>::found>>
    variant(U&& u) {
        using normalized = parameter_normalize<U>;
        using index = pack_find<U>;
        auto p = get<U>();
        detail::construct<U>::exec(p, std::forward<U>(u));
        current = index::value;
    }

    variant& operator=(variant const& v) {
        if (this != &v) {
            destroy_current();
            copy(v);
        }
        return *this;
    }

    variant& operator=(variant&& v) {
        if (this != &v) {
            destroy_current();
            move(std::move(v));
        }
        return *this;
    }

    template<typename U, typename = std::enable_if_t<pack_find<U>::found>>
    variant& operator=(U&& v) {
        destroy_current();
        emplace<U>(std::forward<U>(v));
        return *this;
    }

    ~variant() {
        destroy_current();
    }

    template<typename U, typename... Args>
    void emplace(Args&&... args) {
        assert_pack_explicitly_contains<U>();
        using index = pack_find<U>;
        static_assert(index::found, "variant has no such member");
        destroy_current();
        auto p = get<U>();
        detail::construct<U>::exec(p, std::forward<Args>(args)...);
        current = index::value;
    }

    template<typename U>
    bool contains() const {
        using index = pack_find<U>;
        static_assert(index::found, "variant has no such member");
        return index::value == current;
    }

    template<typename U>
    friend U const& get(variant const& v) {
        assert_pack_explicitly_contains<U>();
        assert(v.template contains<U>() && "member not currently set");
        return v.template get_cref<U>();
    }

    template<typename U>
    friend U& get(variant& v) {
        assert_pack_explicitly_contains<U>();
        assert(v.template contains<U>() && "member not currently set");
        return v.template get_ref<U>();
    }

    template<typename U>
    friend U const* get(variant const* v) {
        assert_pack_explicitly_contains<U>();
        if (v->template contains<U>())
            return &v->template get_cref<U>();
        return nullptr;
    }

    template<typename U>
    friend U* get(variant* v) {
        assert_pack_explicitly_contains<U>();
        if (v->template contains<U>())
            return &v->template get_ref<U>();
        return nullptr;
    }
};

// Necessary, as the friend declarations declare get to be in the scope of variant.
template<typename U, typename... Types>
U const& get(variant<Types...> const&);

#include "details/variant.hpp"

