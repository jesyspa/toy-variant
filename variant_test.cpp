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

#define LOG(stmt) std::cout << ">>> " << #stmt << ";\n"; stmt
#define PRINT(expr) do { std::cout << "--- " << #expr << " == " << (expr) << "\n"; } while (false)

int main() {
    using tracked::B;
    using var = variant<int, B, broken>;
    PRINT(sizeof(var));

    LOG(var v);
    LOG(v.emplace<int>(5));
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
    using var_rec = variant<nil, recursive_helper<cell>>;
    struct cell {
        int head;
        var_rec tail;
    };

    LOG(auto cons = [](int i, var_rec v) -> var_rec { return (cell{i, v}); });
    LOG(auto val = cons(5, cons(6, cons(7, nil{}))));

    LOG(auto total = 0);
    while (auto ptr = get<cell>(&val)) {
        LOG(total += ptr->head);
        LOG(val = ptr->tail);
        PRINT(total);
    }
}
