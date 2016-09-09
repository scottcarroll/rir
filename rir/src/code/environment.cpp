#include "environment.h"
#include "analysis.h"

namespace rir {

Environment Environment::findBinding(SEXP name) {
    throw "NOT IMPLEMENTED";
}

/** Normal variable lookup.
 */
AVALUE Environment::findVariable(SEXP name) {
    assert(good());
    return impl_->findVariable(name, impl_->analysis_.execution().mode());
}

/** Implements R's closure lookup.
 */
AVALUE Environment::findClosure(SEXP name) {
    assert(good());
    return impl_->findClosure(name, impl_->analysis_.execution().mode());
}

/** Assignment into the environment.
 */
void Environment::assign(SEXP name, AVALUE value) {
    assert(good());
    impl_->assign(name, value, impl_->analysis_.execution().mode());
}

/** R's super assignment.

  Obtains parent of the environment and performs a super assignment in it.
 */
void Environment::superassign(SEXP name, AVALUE value) {
    assert(good());
    impl_->parent()->superassign(name, value, impl_->analysis_.execution().mode());
}



} // namespace rir
