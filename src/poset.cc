#include "poset.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <queue>
#include <array>

namespace {
    using poset_exn = std::runtime_error;

    template<typename ArgType>
    void push_proper_str(std::ostream& os, ArgType const& arg) {
        if constexpr(std::is_null_pointer<ArgType>::value) {
            os << "NULL";
        }
        else if constexpr(std::is_pointer<ArgType>::value) {
            os << (arg ? arg : "NULL");
        }
        else {
            os << arg;
        }
    }

    template<typename First, typename Second, typename... Tail>
    void push_proper_str(std::ostream& os, First const& first, Second const& second, Tail const&... tail) {
        push_proper_str(os, first);
        push_proper_str(os, second, tail...);
    }

    template<typename... ArgTypes>
    std::string join(ArgTypes const&... args) {
        std::stringstream ss {};
        push_proper_str(ss, args...);
        return ss.str();
    }

    void print_parameters() { }

    template<typename Sole>
    void print_parameters(Sole const& sole) {
        if constexpr(std::is_pointer<Sole>::value) {
            if (sole) {
                push_proper_str(std::cerr, '\"', sole, '\"');
            }
            else {
                push_proper_str(std::cerr, sole);
            }
        }
        else {
            push_proper_str(std::cerr, sole);
        }
    }

    template <typename First, typename Second, typename... Tail>
    void print_parameters(First const& first, Second const& second, Tail const&... tail) {
        print_parameters(first);
        std::cerr << ", ";
        print_parameters(second, tail...);
    }

    template<typename... ArgTypes>
    void print_signature(char const* method, ArgTypes const&... args) {
#ifndef NDEBUG
        std::cerr << method << "(";
        print_parameters(args...);
        std::cerr << ")\n";
#endif
    }

    template<typename... ArgTypes>
    void log(ArgTypes const&... args) {
#ifndef NDEBUG
        push_proper_str(std::cerr, args..., '\n');
#endif
    }

    using poset_id_t = unsigned long;
    using persistent_value_t = std::string;

    using value_repr_t = uint64_t;
    using names_assoc_t = std::unordered_map<persistent_value_t, value_repr_t>;

    enum { _predecessors = 0, _successors = 1 };
    using node_t = std::pair<std::unordered_set<value_repr_t>, std::unordered_set<value_repr_t>>;

    using nodes_t = std::unordered_map<value_repr_t, node_t>;
    enum { _value_repr_gen = 0, _names_assoc = 1, _nodes = 2 };
    using poset_t = std::tuple<value_repr_t, names_assoc_t, nodes_t>;

    using posets_t = std::unordered_map<poset_id_t, poset_t>;

    poset_id_t poset_id_gen = 0;
    posets_t posets = {};

    unsigned long true_poset_new(char const* method) {
        posets[poset_id_gen] = { 0, {}, {} };

        log(method,
            ": poset ", poset_id_gen,
            " created");

        return poset_id_gen++;
    }

    posets_t::iterator seek_poset(unsigned long id) {
        auto poset_iter = posets.find(id);
        if (poset_iter == posets.end()) {
            throw poset_exn(join(
                "poset ", id,
                " does not exist"));
        }
        else {
            return poset_iter;
        }
    }

    void true_poset_delete(char const* method, unsigned long id) {
        auto poset_iter = seek_poset(id);
        posets.erase(poset_iter);

        log(method,
            ": poset ", id,
            " deleted");
    }

    size_t true_poset_size(char const* method, unsigned long id) {
        auto& nodes = std::get<_nodes>(seek_poset(id)->second);
        auto size = nodes.size();

        log(method,
            ": poset ", id,
            " contains ", size, " element(s)");

        return size;
    }

    template<typename ParamType, typename ArgType>
    void validate_aux(std::vector<poset_exn>& exns, ParamType const& param, ArgType const& arg) {
        if (!arg) {
            exns.emplace_back(join("invalid ", param, " (", arg, ")"));
        }
    }

    template<typename ParamType, typename ArgType, typename... Tail>
    void validate_aux(std::vector<poset_exn>& exns, ParamType const& param, ArgType const& arg,
            Tail const&... tail) {
        validate_aux(exns, param, arg);
        validate_aux(exns, tail...);
    }

    template<typename... All>
    void validate(All const&... all) {
        auto exns = std::vector<poset_exn> {};
        validate_aux(exns, all...);
        if (!exns.empty()) {
            throw exns;
        }
    }

    bool true_poset_insert(char const* method, unsigned long id, char const* value) {
        validate("value", value);

        auto& [value_repr_gen, names_assoc, nodes] = seek_poset(id)->second;
        if (names_assoc.find(value) != names_assoc.end()) {
            throw poset_exn(join(
                "poset ", id,
                ", element \"", value, "\"",
                " already exists"));
        }

        names_assoc[value] = value_repr_gen;
        nodes[value_repr_gen] = { {}, {} };
        ++value_repr_gen;

        log(method,
            ": poset ", id,
            ", element \"", value, "\"",
            " inserted");
        return true;
    }

    using node_iter_set = std::pair<names_assoc_t::iterator, nodes_t::iterator>;
    node_iter_set seek_node(posets_t::iterator poset_iter, char const* value) {
        auto& [_1, names_assoc, nodes] = poset_iter->second;

        auto repr_iter = names_assoc.find(value);
        if (repr_iter == names_assoc.end()) {
            throw poset_exn(join(
                "poset ", poset_iter->first,
                ", element \"", value, "\"",
                " does not exist"));
        }

        return { repr_iter, nodes.find(repr_iter->second) };
    }

    std::array<node_iter_set, 2> seek_nodes(posets_t::iterator poset_iter, char const* value1, char const* value2) {
        try {
            return { seek_node(poset_iter, value1), seek_node(poset_iter, value2) };
        }
        catch (poset_exn const& exn) {
            throw poset_exn(join(
                "poset ", poset_iter->first,
                ", element \"", value1, "\" or \"", value2, "\"",
                " does not exist"));
        }
    }

    bool internal_poset_test(nodes_t const& nodes, nodes_t::iterator const& node1_iter,
            nodes_t::iterator const& node2_iter) {
        if (node1_iter->first == node2_iter->first) {
            return true;
        }

        std::queue<value_repr_t> search_queue = {};
        for (auto const& s: node1_iter->second.second) {
            search_queue.push(s);
        }

        while (!search_queue.empty()) {
            auto latest = search_queue.front();
            search_queue.pop();

            if (latest == node2_iter->first) {
                return true;
            }

            for (auto const& s: nodes.at(latest).second) {
                search_queue.push(s);
            }
        }

        return false;
    }

    void add_cartesian_product(nodes_t& nodes, std::unordered_set<value_repr_t>& pred,
            std::unordered_set<value_repr_t>& succ) {
        for (auto const& p: pred) {
            for (auto const& s: succ) {
                nodes.at(p).second.insert(s);
                nodes.at(s).first.insert(p);
            }
        }
    }

    bool true_poset_remove(char const* method, unsigned long id, char const* value) {
        validate("value", value);

        auto poset_iter = seek_poset(id);
        auto& [_1, names_assoc, nodes] = poset_iter->second;
        auto [repr_iter, node_iter] = seek_node(poset_iter, value);

        add_cartesian_product(nodes, node_iter->second.first, node_iter->second.second);
        nodes.erase(node_iter);
        names_assoc.erase(repr_iter);

        log(method,
            ": poset ", id,
            ", element \"", value, "\"",
            " removed");
        return true;
    }

    bool true_poset_add(char const* method, unsigned long id, char const* value1, char const* value2) {
        validate("value1", value1, "value2", value2);

        auto poset_iter = seek_poset(id);
        auto const& nodes = std::get<_nodes>(poset_iter->second);
        auto [node1_iterset, node2_iterset] = seek_nodes(poset_iter, value1, value2);
        auto [repr1_iter, node1_iter] = node1_iterset;
        auto [repr2_iter, node2_iter] = node2_iterset;

        if (node2_iter->first == node1_iter->first ||
                internal_poset_test(nodes, node1_iter, node2_iter) ||
                internal_poset_test(nodes, node2_iter, node1_iter)) {
            throw poset_exn(join(
                "poset ", id,
                ", relation (\"", value1, "\", \"", value2, "\")",
                " cannot be added"));
        }

        node1_iter->second.second.insert(node2_iter->first);
        node2_iter->second.first.insert(node1_iter->first);

        log(method,
            ": poset ", id,
            ", relation (\"", value1, "\", \"", value2, "\")",
            " added");
        return true;
    }

    bool true_poset_del(char const* method, unsigned long id, char const* value1, char const* value2) {
        validate("value1", value1, "value2", value2);

        auto poset_iter = seek_poset(id);
        auto [node1_iterset, node2_iterset] = seek_nodes(poset_iter, value1, value2);
        auto [repr1_iter, node1_iter] = node1_iterset;
        auto [repr2_iter, node2_iter] = node2_iterset;

        auto& [pred1, succ1] = node1_iter->second;
        auto& [pred2, succ2] = node2_iter->second;

        auto node1_in_pred2_iter = pred2.find(node1_iter->first);
        if (node1_iter->first == node2_iter->first ||
                node1_in_pred2_iter == pred2.end()) {
            throw poset_exn(join(
                "poset ", id,
                ", relation (\"", value1, "\", \"", value2, "\")",
                " cannot be deleted"));
        }
        auto node2_in_succ1_iter = succ1.find(node2_iter->first);
        auto& nodes = std::get<_nodes>(poset_iter->second);

        pred2.erase(node1_in_pred2_iter);
        succ1.erase(node2_in_succ1_iter);
        add_cartesian_product(nodes, pred1, succ1);
        add_cartesian_product(nodes, pred2, succ2);

        log(method,
            ": poset ", id,
            ", relation (\"", value1, "\", \"", value2,"\")",
            " deleted");
        return true;
    }

    bool true_poset_test(char const* method, unsigned long id, char const* value1, char const* value2) {
        validate("value1", value1, "value2", value2);

        auto poset_iter = seek_poset(id);
        auto const& nodes = std::get<_nodes>(poset_iter->second);
        auto [node1_iterset, node2_iterset] = seek_nodes(poset_iter, value1, value2);

        auto rel_exists = internal_poset_test(nodes, node1_iterset.second, node2_iterset.second);

        log(method,
            ": poset ", id,
            ", relation (\"", value1, "\", \"", value2, "\")",
            rel_exists ? " exists" : " does not exist");
        return rel_exists;
    }

    void true_poset_clear(char const* method, unsigned long id) {
        seek_poset(id)->second = { 0, {}, {} };

        log(method,
            ": poset ", id,
            " cleared");
    }
}

template<typename ReturnType, typename... ArgTypes>
ReturnType universal_proc(char const* method, ReturnType(*proc)(char const*, ArgTypes...), ReturnType error_retv,
        ArgTypes... args) {
    try {
        print_signature(method, args...);
        return proc(method, args...);
    }
    catch (poset_exn const& exn) {
        log(method, ": ", exn.what());
        return error_retv;
    }
    catch (std::vector<poset_exn> const& exns) {
        for (poset_exn const& exn: exns) {
            log(method, ": ", exn.what());
        }
        return error_retv;
    }
}

template<typename... ArgTypes>
void universal_proc(char const* method, void(*proc)(char const*, ArgTypes...), ArgTypes... args) {
    try {
        print_signature(method, args...);
        return proc(method, args...);
    }
    catch (poset_exn const& exn) {
        log(method, ": ", exn.what());
    }
    catch (std::vector<poset_exn> const& exns) {
        for (poset_exn const& exn: exns) {
            log(method, ": ", exn.what());
        }
    }
}

unsigned long jnp1::poset_new() {
    return universal_proc<unsigned long>("poset_new", &true_poset_new, (unsigned long)0);
}

void jnp1::poset_delete(unsigned long id) {
    universal_proc("poset_delete", &true_poset_delete, id);
}

size_t jnp1::poset_size(unsigned long id) {
    return universal_proc("poset_size", &true_poset_size, (size_t)0, id);
}

bool jnp1::poset_insert(unsigned long id, char const* value) {
    return universal_proc("poset_insert", &true_poset_insert, false, id, value);
}

bool jnp1::poset_remove(unsigned long id, char const* value) {
    return universal_proc("poset_remove", &true_poset_remove, false, id, value);
}

bool jnp1::poset_add(unsigned long id, char const* value1, char const* value2) {
    return universal_proc("poset_add", &true_poset_add, false, id, value1, value2);
}

bool jnp1::poset_del(unsigned long id, char const* value1, char const* value2) {
    return universal_proc("poset_del", &true_poset_del, false, id, value1, value2);
}

bool jnp1::poset_test(unsigned long id, char const* value1, char const* value2) {
    return universal_proc("poset_test", &true_poset_test, false, id, value1, value2);
}

void jnp1::poset_clear(unsigned long id) {
    universal_proc("poset_clear", &true_poset_clear, id);
}