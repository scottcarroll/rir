#ifndef SAFE_BUILTINS_LIST_H
#define SAFE_BUILTINS_LIST_H

#include "R/r.h"

namespace rir {
namespace pir {

class SafeBuiltinsList {
  public:
    static bool always(SEXP builtin);
    static bool nonObject(SEXP builtin);
    static bool always(int builtin);
    static bool nonObject(int builtin);
};

} // namespace pir
} // namespace rir

#endif