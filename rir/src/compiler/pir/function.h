#ifndef COMPILER_FUNCTION_H
#define COMPILER_FUNCTION_H

#include "R/r.h"
#include "code.h"
#include "pir.h"

namespace rir {
namespace pir {

/*
 * Function
 *
 * A function does not have an environment per se, but just a number of named
 * arguments. If an environment is necessary, `MkEnv` can bind arguments
 * (referred to by `LdArg`).
 *
 */
class Function : public Code {
  public:
    Function(std::initializer_list<SEXP> a, std::initializer_list<Promise*> p)
        : argNames(a), defaultArgs(p) {}

    Function(const std::vector<SEXP>& a, const std::vector<Promise*>& p)
        : argNames(a), defaultArgs(p) {}

    Function(std::initializer_list<SEXP> a) : Function(a, {}) {}
    Function(const std::vector<SEXP>& a) : Function(a, {}) {}

    std::vector<SEXP> argNames;
    std::vector<Promise*> defaultArgs;

    std::vector<Promise*> promises;

    void print(std::ostream& out);

    Promise* createProm();

    size_t maxBBId = 0;

    friend std::ostream& operator<<(std::ostream& out, const Function& e) {
        out << "Func(" << (void*)&e << ")";
        return out;
    }

    ~Function();
};
}
}

#endif
