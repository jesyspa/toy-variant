#pragma once

#include <cassert>
#include <type_traits>

namespace detail {
using destroy_func = void(*)(void*);
using copy_func = void(*)(void*, void const*);
using move_func = void(*)(void*, void*);

template<typename U>
struct internal_error;

template<typename...>
union variant_data;

template<typename U, typename... Ts>
struct pack_find;

template<typename U, typename... Ts>
struct pack_contains;

template<typename U, typename... Ts>
struct assert_pack_contains;

template<typename... Ts>
struct has_duplicates;

template<typename U>
struct member_representation;

template<typename U>
struct normalize;

template<typename T>
struct construct_object;

template<typename T>
struct copy_object;

template<typename T>
struct move_object;

template<typename T>
struct destroy_object;
}

template<typename... Types>
class variant {
    template<typename U>
    using normalize = typename detail::normalize<U>::type;
    template<typename U>
    using pack_contains = detail::pack_contains<U, Types...>;
    template<typename U>
    using assert_pack_contains = detail::assert_pack_contains<normalize<U>, normalize<Types>...>;
    template<typename U>
    using assert_pack_explicitly_contains = detail::assert_pack_contains<U, Types...>;
    template<typename U>
    using pack_find = detail::pack_find<U, Types...>;

    static_assert(!detail::has_duplicates<normalize<Types>...>::value, "cannot have duplicate types in variant");

    using storage_type = detail::variant_data<Types...>;
    storage_type data;
    int current = sizeof...(Types);

    template<typename U>
    void const* cget_impl(std::true_type) const {
        return data.template cget<U>();
    }

    template<typename U>
    void const* cget_impl(std::false_type) const {
        assert(!"internal error: get on non-existent type");
        return nullptr;
    }

    template<typename U>
    void const* cget() const {
        return cget_impl<normalize<U>>(pack_contains<U>{});
    }

    template<typename U>
    void const* get() const {
        return cget<U>();
    }

    template<typename U>
    void* get() {
        return const_cast<void*>(cget<U>());
    }

    void const* cget(int n) const {
        return data.cget(n);
    }

    void const* get(int n) const {
        return cget(n);
    }

    void* get(int n) {
        return const_cast<void*>(cget(n));
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

    template<typename U>
    bool contains_impl(std::true_type) {
        return pack_find<U>::value == current;
    }

    template<typename U>
    bool contains_impl(std::false_type) {
        assert(!"internal error: contains on non-existent type");
        return false;
    }

    void destroy_current() {
        if (current == sizeof...(Types))
            return;
        auto p = get(current);
        auto f = storage_type::get_destructor(current);
        f(p);
        current = sizeof...(Types);
    }

    template<typename U, typename... Args>
    void emplace_impl(Args&&... args) {
        using index = pack_find<U>;
        destroy_current();
        auto p = get<U>();
        detail::construct_object<U>::exec(p, std::forward<Args>(args)...);
        current = index::value;
    };

    void copy(variant const& v) {
        if (v.current == sizeof...(Types))
            return;
        copy(v.current, v.get(v.current));
        current = v.current;
    }

    void move(variant&& v) {
        if (v.current == sizeof...(Types))
            return;
        move(v.current, v.get(v.current));
        current = v.current;
    }

    void copy(int n, void const* other) {
        auto f = storage_type::get_copy_constructor(n);
        auto p = get(n);
        f(p, other);
    }

    void move(int n, void* other) {
        auto f = storage_type::get_move_constructor(n);
        auto p = get(n);
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

    template<typename U, typename = std::enable_if_t<pack_contains<U>::value>>
    variant(U&& u) {
        using index = pack_find<U>;
        using E = typename index::type;
        auto p = get<E>();
        detail::construct_object<E>::exec(p, std::forward<U>(u));
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

    template<typename U, typename = std::enable_if_t<pack_contains<U>::value>>
    variant& operator=(U&& v) {
        emplace_impl<U>(std::forward<U>(v));
        return *this;
    }

    ~variant() {
        destroy_current();
    }

    template<typename U, typename... Args>
    void emplace(Args&&... args) {
        assert_pack_explicitly_contains<U>();
        emplace_impl<U>(std::forward<Args>(args)...);
    }

    template<typename U>
    bool contains() const {
        auto flag = assert_pack_explicitly_contains<U>{};
        return contains_impl<U>(flag);
    }

    template<typename U>
    friend U const& get(variant const& v) {
        auto flag = assert_pack_explicitly_contains<U>{};
        (void)flag;
        assert(v.template contains_impl<U>(flag) && "member not currently set");
        return v.template get_cref<U>();
    }

    template<typename U>
    friend U& get(variant& v) {
        auto flag = assert_pack_explicitly_contains<U>{};
        assert(v.template contains_impl<U>(flag) && "member not currently set");
        return v.template get_ref<U>();
    }

    template<typename U>
    friend U const* get(variant const* v) {
        auto flag = assert_pack_explicitly_contains<U>{};
        if (v->template contains_impl<U>(flag))
            return &v->template get_cref<U>();
        return nullptr;
    }

    template<typename U>
    friend U* get(variant* v) {
        auto flag = assert_pack_explicitly_contains<U>{};
        if (v->template contains_impl<U>(flag))
            return &v->template get_ref<U>();
        return nullptr;
    }
};

// Necessary, as the friend declarations declare get to be in the scope of variant.
template<typename U, typename... Types>
U const& get(variant<Types...> const&);

#include "details/variant.hpp"

