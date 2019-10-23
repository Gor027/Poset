#include "poset.h"
#include <unordered_set>
#include <unordered_map>
#include <string>

// These types are prescribed by the interface.
using poset_id_t = unsigned long;
using poset_value_t = char const*;

// Onto the data types proper.

// If I remember correctly, we are specifically not to store more than one copy of any poset value. That, obviously,
// poses some difficulties; my idea is that we could perhaps introduce some injective map from the values to it's
// representative, presumably having lower memory overhead (I chose uint32_t, since it holds ~4*10^9 values, so
// enough for most uses). Then, we essentially map such values to the representatives via value_repr_store_t and
// operate on those. As far as uniqueness, I was thinking about keeping a counter that one would increase when
// inserting values, and when the counter would reach its' maximum (or rather IF, since that max is ~4*10^9), we'd
// introduce some sort of "reordering", like reinserting all the keys present, which would be amortized O(1), and
// usually 0 since we usually would not reach ~4*10^9 insertions.
using poset_value_instance_t = std::string;
using value_repr_t = uint32_t;
using value_repr_store_t = std::tuple<value_repr_t, std::unordered_map<poset_value_instance_t, value_repr_t>>;

// As far as the posets themselves are concerned, it's clear that they are disjoint sets of DAGs; I am thinking about
// simply keeping, for each item in poset, a set of their successors, and operate on poset that way. The whole
// structure, i.e. posets_t, would be an aforementioned injective map and the assoc from posets ids' to appropriate
// posets. Now, in poset_new we essentially get ids from thin air, so I recommend using a counter or something (with
// all the aforementioned issues.
using poset_t = std::unordered_map<value_repr_t, std::unordered_set<value_repr_t>>;
using posets_t = std::tuple<poset_id_t, value_repr_store_t, std::unordered_map<poset_id_t, poset_t>>;

// That is the main data structure for our problem (we must use globals since the interface does not accomodate any
// additional arguments). Now, there may (and probably will) be some linking issues, but I shall have to research that.
posets_t posets;

// Inasfar as the impls are concerned, we shall discuss that later.

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