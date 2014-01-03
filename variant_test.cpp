#include "variant.hpp"
#include <iostream>
#include <exception>

namespace tracked {
using std::cout;

struct B {
    static int next_id;

    B() : id(next_id++) { cout << "*B" << id << "\n"; }
    B(B const& b) : id(next_id++) { cout << "*B" << id << "(B" << b.id << ")\n"; }
    B(B&& b) : id(next_id++) { cout << "*B" << id << " <- B" << b.id << "\n"; }
    B& operator=(B const& b) {
        cout << "B" << id << " = B" << b.id << "\n";
        return *this;
    }
    B& operator=(B&& b) {
        cout << "B" << id << " <= B" << b.id << "\n";
        return *this;
    }
    ~B() { cout << "~B" << id << "\n"; }

    int id;
};

int B::next_id;
}

struct broken {
    struct exception : std::exception {};
    broken() {
        throw exception{};
    }
};

#define LOG(stmt)                                                                                                      \
    std::cout << ">>> " << #stmt << ";\n";                                                                             \
    stmt
#define PRINT(expr)                                                                                                    \
    do {                                                                                                               \
        std::cout << "--- " << #expr << " == " << (expr) << "\n";                                                      \
    } while (false)

int main() {
    using tracked::B;
    using var = variant<int, B, broken>;
    PRINT(sizeof(var));

    LOG(var v);
    LOG(v.emplace<int>(5));
    try {
        LOG(get<B>(v));
    }
    catch (invalid_get_error&) {
        std::cout << "recovered from exception\n";
    }

    PRINT(get<int>(v));
    LOG(v.emplace<B>());
    LOG(var v2 = B{});

    try {
        LOG(v2.emplace<broken>());
    }
    catch (broken::exception&) {
        std::cout << "recovered from exception\n";
    }

    LOG(v2.emplace<B>());
    LOG(v = v2);
    LOG(v2 = 7);
    PRINT(get<int>(v2));
    LOG(v2 = std::move(v));
    LOG(int const i = 1);
    LOG(var v3(i));
    LOG(auto p = get<int>(&v3));
    if (p)
        PRINT(*p);
    else
        PRINT(p);

    using var_ref = variant<int&, B>;
    PRINT(sizeof(var_ref));
    LOG(var_ref vr);
    LOG(int j = 6);
    LOG(vr.emplace<int&>(j));
    PRINT(get<int&>(vr));
    LOG(get<int&>(vr) = 8);
    PRINT(get<int&>(vr));
    PRINT(j);

    using var_const = variant<int const, B const>;
    LOG(var_const vc);
    LOG(vc = 5);
    PRINT(get<int const>(vc));
    LOG(vc.emplace<B const>());

    struct nil {};
    struct cell;
    using var_list = variant<nil, recursive_helper<cell>>;
    struct cell {
        int head;
        var_list tail;
    };

    LOG(auto cons = [](int i, var_list v)->var_list { return (cell{i, v}); });
    LOG(auto val = cons(5, cons(6, cons(7, nil{}))));

    LOG(auto total = 0);
    while (auto ptr = get<cell>(&val)) {
        LOG(total += ptr->head);
        LOG(val = ptr->tail);
        PRINT(total);
    }

    LOG(val = cons(1, cons(4, nil{})));

    struct list_sum_visitor : visitor<int> {
        int operator()(nil) const { return 0; }

        int operator()(cell const& c) const { return c.head + apply_visitor(*this, c.tail); }
    };
    PRINT(apply_visitor(list_sum_visitor{}, val));

    struct stateful_sum_visitor : visitor<void> {
        int visits = 0;
        int sum = 0;

        void operator()(nil) { visits += 1; }

        void operator()(cell const& c) {
            visits += 1;
            sum += c.head;
            apply_visitor(*this, c.tail);
        }
    };
    LOG(stateful_sum_visitor visitor);
    LOG(apply_visitor(visitor, val));
    PRINT(visitor.visits);
    PRINT(visitor.sum);
}
