#pragma once

#include <cassert>
#include <exception>
#include <type_traits>
#include <utility>

namespace detail {
using destroy_func = void (*)(void*);
using copy_func = void (*)(void*, void const*);
using move_func = void (*)(void*, void*);

template <typename U>
struct internal_error;

template <typename...>
union variant_data;

// Search helpers
template <template <typename> class NORM, typename T, typename... Pack>
struct pack_find;

template <template <typename> class NORM, typename T, typename... Pack>
struct pack_contains;

template <template <typename> class NORM, typename T, typename... Pack>
struct assert_pack_contains;

template <template <typename> class NORM, typename... Pack>
struct has_duplicates;

template <typename T>
struct is_recursive;

// Normal forms.  We have six:
//   - decl-form is the form in which the user passes us types when defining the
//     variant.  These types are of the form:
//          T, T const, T&, T const&, rh<T>, rh<T const>
//     where rh is recursive_helper.
//   - query-form is the form in which the user can explicitly query a type.
//          T, T const, T&, T const&
//     the one used to query must match what is declared.
//   - store-form is the form given to the union to use for storage, and the
//     form used to specify operations.
//          T, T&, rh<T>
//   - unique-form is the form by which every element in the variant must be
//     unique.  All searches eventually go by this form, once the validity has
//     been established.
//   - member-form is the form in which the member is internally represented.
//     This is used by the union, but the variant doesn't care about it.
//
// The following conversions are valid, as are their compositions:
//      decl -> query
//      decl -> store
//      query -> unique
//      store -> unique
//      store -> member
//
// We suffix our types with their category.  (This naming convention is better
// than nothing but still not very enlightening; suggestions welcome.)

template <typename T>
struct query_form;

template <typename T>
struct store_form;

template <typename T>
struct unique_form;

template <typename T>
struct member_form;

// Runtime helpers.  These provide functions for right way to operate on types
// at runtime.
template <typename TStore>
struct operations;
}

template <typename T>
struct recursive_helper {
    static_assert(!std::is_reference<T>::value, "no need for recursive helper on references");
};

template <typename Ret>
struct visitor {
    using result_type = Ret;
};

struct empty_variant_error : std::exception {};
struct invalid_get_error : std::exception {};

template <typename... Types>
class variant {
    template <typename T>
    using query_form = typename detail::query_form<T>::type;
    template <typename T>
    using store_form = typename detail::store_form<T>::type;
    template <typename T>
    using unique_form = typename detail::unique_form<T>::type;
    template <typename T>
    using query = detail::pack_find<detail::query_form, T, Types...>;
    template <typename T>
    using can_query = detail::pack_contains<detail::query_form, T, Types...>;
    template <typename T>
    using find = detail::pack_find<detail::unique_form, T, Types...>;
    template <typename T>
    using can_find = detail::pack_contains<detail::unique_form, T, Types...>;
    template <typename T>
    using assert_pack_contains = detail::assert_pack_contains<detail::unique_form, T, Types...>;
    template <typename T>
    using assert_pack_explicitly_contains = detail::assert_pack_contains<detail::query_form, T, Types...>;

    static_assert(!detail::has_duplicates<detail::unique_form, Types...>::value,
                  "cannot have duplicate types in variant");

    using storage_type = detail::variant_data<store_form<Types>...>;
    storage_type data;
    static constexpr int invalid = sizeof...(Types);
    int current = invalid;

    template <typename TDecl>
    void const* cget_impl(std::true_type) const {
        return data.template cget<store_form<TDecl>>();
    }

    template <typename TDecl>
    void const* cget_impl(std::false_type) const {
        assert(!"internal error: get on non-existent type");
        return nullptr;
    }

    template <typename TDecl>
    void const* cget() const {
        return cget_impl<TDecl>(can_query<TDecl>{});
    }

    template <typename TDecl>
    void const* get() const {
        return cget<TDecl>();
    }

    template <typename TDecl>
    void* get() {
        return const_cast<void*>(cget<TDecl>());
    }

    void const* cget(int n) const { return data.cget(n); }

    void const* get(int n) const { return cget(n); }

    void* get(int n) { return const_cast<void*>(cget(n)); }

    template <typename TDecl>
    unique_form<TDecl> const& get_cref() const {
        return detail::operations<TDecl>::as_cref(get<TDecl>());
    }

    template <typename TDecl>
    unique_form<TDecl>& get_ref() {
        return const_cast<unique_form<TDecl>&>(get_cref<TDecl>());
    }

    template <typename TDecl>
    bool contains_impl(std::true_type) {
        return query<TDecl>::value == current;
    }

    template <typename TDecl>
    bool contains_impl(std::false_type) {
        assert(!"internal error: contains on non-existent type");
        return false;
    }

    void destroy_current() {
        if (current == invalid)
            return;
        auto p = get(current);
        auto f = storage_type::get_destructor(current);
        f(p);
        current = invalid;
    }

    bool currently_recursive() const { return current != invalid && data.is_recursive(current); }

    template <typename TQuery, typename... Args>
    void emplace_impl(Args&&... args) {
        using index = find<TQuery>;
        using TDecl = typename index::type;
        destroy_current();
        auto p = get<TDecl>();
        detail::operations<store_form<TDecl>>::construct(p, std::forward<Args>(args)...);
        current = index::value;
    }

    void copy(variant const& v) {
        if (v.current == invalid)
            return;
        copy(v.current, v.get(v.current));
        current = v.current;
    }

    void move(variant&& v) {
        if (v.current == invalid)
            return;
        move(v.current, v.get(v.current));
        current = v.current;
        v.current = invalid;
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

    variant(variant const& v) { copy(v); }

    variant(variant&& v) { move(std::move(v)); }

    template <typename TQuery, typename = std::enable_if_t<can_find<TQuery>::value>>
    variant(TQuery&& x) {
        using index = find<TQuery>;
        using TDecl = typename index::type;
        auto p = get<TDecl>();
        detail::operations<store_form<TDecl>>::construct(p, std::forward<TQuery>(x));
        current = index::value;
    }

    variant& operator=(variant const& v) {
        if (this != &v) {
            if (currently_recursive()) {
                auto v2(v);
                destroy_current();
                move(std::move(v2));
            } else {
                destroy_current();
                copy(v);
            }
        }
        return *this;
    }

    variant& operator=(variant&& v) {
        if (this != &v) {
            if (currently_recursive()) {
                auto v2(std::move(v));
                destroy_current();
                move(std::move(v2));
            } else {
                destroy_current();
                move(std::move(v));
            }
        }
        return *this;
    }

    template <typename TQuery, typename = std::enable_if_t<can_find<TQuery>::value>>
    variant& operator=(TQuery&& v) {
        emplace_impl<TQuery>(std::forward<TQuery>(v));
        return *this;
    }

    ~variant() { destroy_current(); }

    template <typename TQuery, typename... Args>
    void emplace(Args&&... args) {
        assert_pack_explicitly_contains<TQuery>();
        emplace_impl<TQuery>(std::forward<Args>(args)...);
    }

    template <typename TQuery>
    bool contains() const {
        auto flag = assert_pack_explicitly_contains<TQuery>{};
        return contains_impl<TQuery>(flag);
    }

    template <typename Visitor>
    auto apply_visitor(Visitor&& vis) const -> typename std::remove_reference<Visitor>::type::result_type {
        if (current == invalid)
            throw empty_variant_error{};
        return data.apply(std::forward<Visitor>(vis), current);
    }

    template <typename Visitor>
    auto apply_visitor(Visitor&& vis) -> typename std::remove_reference<Visitor>::type::result_type {
        if (current == invalid)
            throw empty_variant_error{};
        return data.apply(std::forward<Visitor>(vis), current);
    }

    template <typename TQuery>
    friend unique_form<TQuery> const& get(variant const& v) {
        using TDecl = typename query<TQuery>::type;
        auto flag = assert_pack_explicitly_contains<TQuery>{};
        if (v.template contains_impl<TDecl>(flag))
            return v.template get_cref<TDecl>();
        throw invalid_get_error{};
    }

    template <typename TQuery>
    friend unique_form<TQuery>& get(variant& v) {
        using TDecl = typename query<TQuery>::type;
        auto flag = assert_pack_explicitly_contains<TQuery>{};
        if (v.template contains_impl<TDecl>(flag))
            return v.template get_ref<TDecl>();
        throw invalid_get_error{};
    }

    template <typename TQuery>
    friend unique_form<TQuery> const* get(variant const* v) {
        using TDecl = typename query<TQuery>::type;
        auto flag = assert_pack_explicitly_contains<TQuery>{};
        if (v->template contains_impl<TDecl>(flag))
            return &v->template get_cref<TDecl>();
        return nullptr;
    }

    template <typename TQuery>
    friend unique_form<TQuery>* get(variant* v) {
        using TDecl = typename query<TQuery>::type;
        auto flag = assert_pack_explicitly_contains<TQuery>{};
        if (v->template contains_impl<TDecl>(flag))
            return &v->template get_ref<TDecl>();
        return nullptr;
    }
};

// Necessary, as the friend declarations declare get to be in the scope of variant.
template <typename U, typename... Types>
U const& get(variant<Types...> const&);

template <typename Visitor, typename Variant>
auto apply_visitor(Visitor&& vis, Variant&& var) -> typename std::remove_reference<Visitor>::type::result_type {
    return var.apply_visitor(std::forward<Visitor>(vis));
}

#include "details/variant.hpp"

