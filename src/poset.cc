#include "poset.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <tuple>

// We shall start with the interface types, namely ids and values; since char const* cannot be stored in the usual
// way, we shall store proxies for them (in this case, STL strings).
using poset_id_t = unsigned long;
using value_t = char const*;
using persistent_value_t = std::string;

// Now, onto the overall structure of the project.
//     We shall maintain some "program state" variable, i.e. posets_t for all stored posets and auxiliary stuff. Given
// how we need to come up with poset id's from essentially thin air in poset_new, we need a method for generating
// unique poset_id_t's. I propose following scheme: to store a counter of some type convertable to poset_id_t, and when
// adding new posets we would yield the counter's value and increment it - of course, then the generated id's are unique.
// That, however, presents an issue: what to do when counter reaches its max value? Then, I suppose, we could reassign
// ids - practically, however, if the counter is uint64_t, then we'd have to insert ~1.8*10^19 items to reach the
// max, so it may be altogether unnecessary of us to even concern ourselves with reassignments etc. We may discuss whether
// we want theoretical correctness or practical convenience later on.
//     So, posets_t is essentially counter + map from poset ids' to posets. We are, clearly, lacking structure for a
// poset. In general, we could consider it to be a graph of some sort, i.e. something corresponding to a map from node
// id to a store of successors' ids, and merely ensure that the operations modifying the poset preserve its "posetness"
// Naively, we could just consider ids to be persistent_value_t's. We are, however, given an explicit requirement for
// storing the values only once across the program. My idea, then, is to represent values by some other type (by setting
// node id's type to that type and introducing a map from values to the representatives. We encounter, once again, a
// problem of taking representatives from thin air, but we can again utilise the counter system. Therefore, have:

using value_repr_t = uint64_t;
using poset_t = std::unordered_map<value_repr_t, std::unordered_set<value_repr_t>>;

enum { _repr_counter = 0, _names_to_repr = 1 };
using names_store_t = std::tuple<value_repr_t, std::unordered_map<persistent_value_t, value_repr_t>>;

enum { _id_counter = 0, _id_to_poset = 1 };
using posets_store_t = std::tuple<poset_id_t, std::unordered_map<poset_id_t, poset_t>>;

enum { _names_store = 0, _posets_store = 1 };
using posets_t = std::tuple<names_store_t, posets_store_t>;

// Some notes about implementation proper: in the task statement, we are to ensure that name is stored only once for
// each poset, here we store only once globally. Now, this approach seems better, but perhaps we could ascertain that
// on the forum. Also, if you wonder what are enums doing here, it's a trick for "more semantic" extraction of data
// from tuples, as opposed to passing magic numbers to std::get<...>. (As an alternative, we could use structured
// bindings, when we extract everything from the tuple).

// The "global state" of our program; we use static in accordance to what I was talking about earlier about not
// polluting linker's namespaces. Now, I'm pretty sure that's how you do it, but I'd have to check up on that some more.
// Also, functions extraneous to the interface should also be declared static, for the same reason.
static posets_t posets = {
    { 0, {} }, // Names store initialization
    { 0, {} }  // Posets store initialization
};

unsigned long jnp1::poset_new(void) {
    return 0;
}

void jnp1::poset_delete(unsigned long id) {

}

size_t jnp1::poset_size(unsigned long id) {
    return 0;
}

bool jnp1::poset_insert(unsigned long id, char const* value) {
    return false;
}

bool jnp1::poset_remove(unsigned long id, char const* value) {
    return false;
}

bool jnp1::poset_add(unsigned long id, char const* value1, char const* value2) {
    return false;
}

bool jnp1::poset_del(unsigned long id, char const* value1, char const* value2) {
    return false;
}

bool jnp1::poset_test(unsigned long id, char const* value1, char const* value2) {
    return false;
}

void jnp1::poset_clear(unsigned long id) {

}