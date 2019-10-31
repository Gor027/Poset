#include "poset.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <queue>
#include <array>
#include <algorithm>

namespace {
    // We shall start with type decls and static vars.

    // The standard error type for the subprocs.
    using poset_exn = std::runtime_error;

    // Type for poset id (unsigned long as indicated by the task statement).
    using poset_id_t = unsigned long;

    // Type for storing char const*, we chose std::string since it it standard and has the benefit of automatic casts.
    using persistent_value_t = std::string;

    // As requested in the task statement, we are not to store names more than once. Our solution is to associate
    // names with integers (more specifically uint64_t's) and work only on these internally. We shall ensure
    // uniqueness by incrementing the counter at the poset_insert op: that way, one would need to perform ~1.8*10^19
    // such ops to break uniqueness, which we consider impractical.
    using value_repr_t = uint64_t;
    using names_assoc_t = std::unordered_map<persistent_value_t, value_repr_t>;

    // As for the posets, we chose a "graph-like" data structure for these: i.e. have elements as nodes and store
    // with them their select partial-order-relative predecesors and successors; further relations are induced by the
    // implicit transitive and reflexive closure over such a graph.

    // ->first is predecessors, ->second is successors;
    using node_t = std::pair<std::unordered_set<value_repr_t>, std::unordered_set<value_repr_t>>;
    using nodes_t = std::unordered_map<value_repr_t, node_t>;

    // First entity represents a counter for generating unique char const* representants. The other are clear.
    using poset_t = std::tuple<value_repr_t, names_assoc_t, nodes_t>;
    using posets_t = std::unordered_map<poset_id_t, poset_t>;

    // Following two are static variable wrappers, as to ensure proper initialization.
    // A counter for generating poset id's (for more info see comment about value_repr_t).
    poset_id_t& poset_id_gen() {
        static poset_id_t _poset_id_gen = 0;
        return _poset_id_gen;
    }

    // The primary data structure for the task, containing appropriately associated posets.
    posets_t& posets() {
        static posets_t _posets = {};
        return _posets;
    }

    // This is a variable for storing method name for the purposes of logging. (Note that, since we always init
    // method in the interface, we need not pre-initialize it statically.)
    char const* method;

    // The following "scheme" outputs into the stream correctly formatted variables (i.e. "NULL" string if it's
    // NULL, the variable otherwise).
    template<typename ArgType>
    void push_proper_str(std::ostream& os, ArgType const& arg) {
        if constexpr(std::is_pointer<ArgType>::value) {
            os << (arg ? arg : "NULL");
        }
        else {
            os << arg;
        }
    }

    template<typename First, typename... Tail>
    void push_proper_str(std::ostream& os, First const& first, Tail const&... tail) {
        (push_proper_str(os, first), ..., push_proper_str(os, tail));
    }

    // As the name suggests, this function joins arguments into single string.
    template<typename... ArgTypes>
    std::string join(ArgTypes const&... args) {
        std::stringstream ss {};
        push_proper_str(ss, args...);
        return ss.str();
    }

    // This function yields an ostream for logging purposes: specifically, if !NDEBUG, we use std::cerr, otherwise we
    // use a no-op ostream.
    std::ostream& log_stream() {
#ifndef NDEBUG
        return std::cerr;
#else
        static std::ostream null_stream(NULL);
        return null_stream;
#endif
    }

    // The following "scheme" prints into cerr a sequence of properly-formatted arguments of the function signature.
    void print_parameters() { }

    template<typename Sole>
    void print_parameters(Sole const& sole) {
        if constexpr(std::is_pointer<Sole>::value) {
            if (sole) {
                push_proper_str(log_stream(), '\"', sole, '\"');
            }
            else {
                push_proper_str(log_stream(), sole);
            }
        }
        else {
            push_proper_str(log_stream(), sole);
        }
    }

    template <typename First, typename... Tail>
    void print_parameters(First const& first, Tail const&... tail) {
        (print_parameters(first), ..., ( log_stream() << ", ", print_parameters(tail)) );
    }

    template<typename... ArgTypes>
    void print_signature(ArgTypes const&... args) {
        log_stream() << method << "(";
        print_parameters(args...);
        log_stream() << ")\n";
    }

    template<typename... ArgTypes>
    void log(ArgTypes const&... args) {
        push_proper_str(log_stream(), args..., '\n');
    }

    // This, and other procedures with true_ prefix, are versions of "proper" procedures that yield exceptions. We
    // have decided to use this scheme, because, for example with seeking poset, we would have to repeat the same log
    // message multiple times, and that way we simply propagate appropriate errors "up the stream", therefore
    // ensuring consistency and not repeating ourselves.
    unsigned long true_poset_new() {
        posets()[poset_id_gen()] = { 0, {}, {} };

        log(method,
            ": poset ", poset_id_gen(),
            " created");

        return poset_id_gen()++;
    }

    posets_t::iterator seek_poset(unsigned long id) {
        auto poset_iter = posets().find(id);
        if (poset_iter == posets().end()) {
            throw poset_exn(join(
                "poset ", id,
                " does not exist"));
        }
        else {
            return poset_iter;
        }
    }

    void true_poset_delete(unsigned long id) {
        auto poset_iter = seek_poset(id);
        posets().erase(poset_iter);

        log(method,
            ": poset ", id,
            " deleted");
    }

    size_t true_poset_size(unsigned long id) {
        auto& nodes = std::get<nodes_t>(seek_poset(id)->second);
        auto size = nodes.size();

        log(method,
            ": poset ", id,
            " contains ", size, " element(s)");

        return size;
    }

    // The following scheme is used for validating variables: note that we may not simply use a sequence of
    // validations, because they yield errors, and in the example error files we clearly see that, for example, if
    // two NULLs are passed, we need to log both of them, which is complicated were we to use a sequence of
    // validations. Therefore, we shall validate all of variables at the same time and simply yield a vector of
    // errors if anything happens.
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

    bool true_poset_insert(unsigned long id, char const* value) {
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

    // This seek procedure yields not only the appropriate node iter, but also name representative iter, mostly for
    // use in remove as to not find it again.
    using node_iter_set = std::pair<names_assoc_t::iterator, nodes_t::iterator>;
    node_iter_set seek_node(posets_t::iterator const& poset_iter, char const* value) {
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

    // In a similar way to argument validation proc, the structure of error messages imposes a simultaneous checking
    // of these in some particular cases (specifically, if two names are given to proc we may need to say that one or
    // the other is not present, as opposed to some specific one not being present).
    std::array<node_iter_set, 2> seek_nodes(posets_t::iterator const& poset_iter, char const* value1, char const*
            value2) {
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

    // This "internal" version of poset_test specifically does not yield exceptions and accepts "ready" node
    // iterators. As far as the algorithm is concerned, it is mere BFS.
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

    bool true_poset_remove(unsigned long id, char const* value) {
        validate("value", value);

        auto poset_iter = seek_poset(id);
        auto& [_1, names_assoc, nodes] = poset_iter->second;
        auto [repr_iter, node_iter] = seek_node(poset_iter, value);

        // In our graph formulation, value removal is identical to removing a node from the graph: of course,
        // however, merely removing the node is insufficient, as some p \in P(v) and some s \in S(v) may be in
        // relation by the virtue of transitivity through v, and not be "directly in relation" (i.e. connected on
        // graph), wherefore we must preemptively add all such relations. We must also remove v from the successor
        // sets of predecessors and predecesor sets of successors.

        for (auto const& pred: node_iter->second.first) {
            auto& pred_node = nodes.at(pred);
            pred_node.second.erase(node_iter->first);

            for (auto const& succ: node_iter->second.second) {
                auto& succ_node = nodes.at(succ);
                pred_node.second.insert(succ);
                succ_node.first.insert(pred);
            }
        }
        for (auto const& succ: node_iter->second.second) {
            auto& succ_node = nodes.at(succ);
            succ_node.first.erase(node_iter->first);
        }

        nodes.erase(node_iter);
        names_assoc.erase(repr_iter);

        log(method,
            ": poset ", id,
            ", element \"", value, "\"",
            " removed");
        return true;
    }

    bool true_poset_add(unsigned long id, char const* value1, char const* value2) {
        validate("value1", value1, "value2", value2);

        auto poset_iter = seek_poset(id);
        auto& nodes = std::get<nodes_t>(poset_iter->second);
        auto [node1_iterset, node2_iterset] = seek_nodes(poset_iter, value1, value2);
        auto node1_iter = node1_iterset.second, node2_iter = node2_iterset.second;
        auto& succ1 = node1_iter->second.second;
        auto& pred2 = node2_iter->second.first;

        // The relation (u, v) may not be added if either it is present (by virtue of reflexivity (1) or by being
        // path-connected on the poset DAG (2)), or if (v, u) is present and u != v (in which case we would break
        // antisymmetry) (3).
        if (node2_iter->first == node1_iter->first || // (1)
                internal_poset_test(nodes, node1_iter, node2_iter) || // (2)
                internal_poset_test(nodes, node2_iter, node1_iter)) { // (3)
            throw poset_exn(join(
                "poset ", id,
                ", relation (\"", value1, "\", \"", value2, "\")",
                " cannot be added"));
        }

        succ1.insert(node2_iter->first);
        pred2.insert(node1_iter->first);

        log(method,
            ": poset ", id,
            ", relation (\"", value1, "\", \"", value2, "\")",
            " added");
        return true;
    }

    bool true_poset_del(unsigned long id, char const* value1, char const* value2) {
        validate("value1", value1, "value2", value2);

        auto poset_iter = seek_poset(id);
        auto& nodes = std::get<nodes_t>(poset_iter->second);
        auto [node1_iterset, node2_iterset] = seek_nodes(poset_iter, value1, value2);
        auto node1_iter = node1_iterset.second, node2_iter = node2_iterset.second;

        auto& [pred1, succ1] = node1_iter->second;
        auto& [pred2, succ2] = node2_iter->second;

        // We embed the error in lambda to avoid repetition.
        auto raise_error = [id, value1, value2]() -> auto {
            throw poset_exn(join(
                "poset ", id,
                ", relation (\"", value1, "\", \"", value2, "\")",
                " cannot be deleted"));
        };

        // Suppose (u, v) is present; we can remove it only if it does not break the poset. i.e. if (u, v) does not
        // follow from reflexivity (i.e. u == v) (1), or transitivity. Because of the latter, if (u, v) is present
        // and we actually can remove the relation, it must follow from direct path on the graph, since otherwise we
        // would have t: (u, t), (t, v) \in R, and then we could not remove (u, v).

        auto node1_in_pred2_iter = pred2.find(node1_iter->first);
        if (node1_iter->first == node2_iter->first || // if u == v
                node1_in_pred2_iter == pred2.end()) { // if (u, v) is not "direct" on the graph
            raise_error();
        }
        auto node2_in_succ1_iter = succ1.find(node2_iter->first);

        // Here, we shall remove the direct edge (u, v) and see whether there are no aforementioned t's (if there
        // are, we reinstate the prior-removed edge and raise an error).
        pred2.erase(node1_in_pred2_iter);
        succ1.erase(node2_in_succ1_iter);
        if (internal_poset_test(nodes, node1_iter, node2_iter)) {
            pred2.insert(node1_iter->first);
            succ1.insert(node2_iter->first);
            raise_error();
        }

        // Through a similar argument as with poset_remove, we must add relation between u and all S(v) along with
        // all P(u) and v
        for (auto const& succ: succ2) {
            auto& succ_node = nodes.at(succ);
            succ1.insert(succ);
            succ_node.first.insert(node1_iter->first);
        }
        for (auto const& pred: pred1) {
            auto& pred_node = nodes.at(pred);
            pred2.insert(pred);
            pred_node.second.insert(node2_iter->first);
        }

        log(method,
            ": poset ", id,
            ", relation (\"", value1, "\", \"", value2,"\")",
            " deleted");
        return true;
    }

    bool true_poset_test(unsigned long id, char const* value1, char const* value2) {
        validate("value1", value1, "value2", value2);

        auto poset_iter = seek_poset(id);
        auto const& nodes = std::get<nodes_t>(poset_iter->second);
        auto [node1_iterset, node2_iterset] = seek_nodes(poset_iter, value1, value2);

        auto rel_exists = internal_poset_test(nodes, node1_iterset.second, node2_iterset.second);

        log(method,
            ": poset ", id,
            ", relation (\"", value1, "\", \"", value2, "\")",
            rel_exists ? " exists" : " does not exist");
        return rel_exists;
    }

    void true_poset_clear(unsigned long id) {
        seek_poset(id)->second = { 0, {}, {} };

        log(method,
            ": poset ", id,
            " cleared");
    }
}

// At this point we have specified all appropriate procedures, but at this point they yield errors which we must
// resolve in order for these to be usable by programs in C (and also for us to log any appropriate errors). Because
// the appropriate structure is repetitive, we have abstracted it out into pair of templates (one for returning
// procs, one for nonreturning).
template<typename ReturnType, typename... ArgTypes>
ReturnType universal_proc(char const* method_name, ReturnType(*proc)(ArgTypes...), ReturnType error_retv,
        ArgTypes... args) {
    try {
        method = method_name;
        print_signature(args...);
        return proc(args...);
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
void universal_proc(char const* method_name, void(*proc)(ArgTypes...), ArgTypes... args) {
    try {
        method = method_name;
        print_signature(args...);
        proc(args...);
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
    return universal_proc("poset_new", &true_poset_new, (unsigned long)0);
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
