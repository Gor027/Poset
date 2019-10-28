#include "poset.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "poset.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

namespace {
    unsigned long test() {
        unsigned long id = ::jnp1::poset_new();
        ::jnp1::poset_insert(id, "A");
        ::jnp1::poset_insert(id, "B");
        ::jnp1::poset_insert(id, "C");
        ::jnp1::poset_add(id, "A", "B");
        ::jnp1::poset_add(id, "B", "C");
        return id;
    }

    unsigned long id = test();
}

int main() {
    assert(::jnp1::poset_test(id, "A", "B"));
    assert(::jnp1::poset_test(id, "B", "C"));
    assert(::jnp1::poset_test(id, "A", "C"));
    assert(::jnp1::poset_del(id, "A", "B"));
    assert(!::jnp1::poset_test(id, "A", "B"));
    assert(::jnp1::poset_test(id, "B", "C"));
    assert(::jnp1::poset_test(id, "A", "C"));
}
