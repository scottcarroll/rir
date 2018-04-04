#ifndef COMPILER_ENV_H
#define COMPILER_ENV_H

#include "instruction_list.h"
#include "tag.h"
#include "value.h"

#include <iostream>
#include <vector>

namespace rir {
namespace pir {

class Instruction;

/*
 * Statically known envs.
 *
 */
class Env : public Value {
    Env(SEXP rho, Env* parent)
        : Value(RType::env, Tag::Env), rho(rho), parent(parent) {}
    friend class Module;

  public:
    SEXP rho;
    Env* parent;

    static Env* nil() {
        static Env u(nullptr, nullptr);
        return &u;
    }

    static Env* theParent() {
        static Env u(nullptr, nullptr);
        return &u;
    }

    void printRef(std::ostream& out);

    static Env* Cast(Value* v) {
        return v->tag == Tag::Env ? static_cast<Env*>(v) : nullptr;
    }

    static bool isPirEnv(Value* v);
    static bool isStaticEnv(Value* v);
    static bool isAnyEnv(Value* v);

    static bool isParentEnv(Value* a, Value* b);
    static Value* parentEnv(Value* e);
};
}
}

#endif