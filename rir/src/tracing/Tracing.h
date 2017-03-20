//
// Created by ksiek on 14.03.17.
//

#ifndef RIR_TRACING_H
#define RIR_TRACING_H

#ifdef __cplusplus

#include <cassert>

#include "R/r.h"

namespace rir {

class Tracing {

  public:
    static Tracing& instance() {
        static Tracing singleton;
        return singleton;
    }

#endif

    enum Type {
        RIR_TRACE_CALL,
        RIR_TRACE_BUILTIN,
        RIR_TRACE_SPECIAL,
        RIR_TRACE_PROMISE_CREATE,
        RIR_TRACE_PROMISE_FORCE,
        RIR_TRACE_PROMISE_LOOKUP,
        RIR_TRACE_NUM_OF
    };

#ifdef __cplusplus

    void addTracer(Type type, SEXP function) {
        if (tracer[type]) {
            Rf_warning("Overwriting existing tracer.");

            // Tell garbage collector to eat this now.
            R_ReleaseObject(tracer[type]);
        }

        // Tell garbage collector not to eat this.
        R_PreserveObject(function);

        tracer[type] = function;
    }

    SEXP getTracer(Type type) {
        assert(type < RIR_TRACE_NUM_OF);
        return tracer[type];
    }

    void unsetTracer(Type type) {
        if (tracer[type]) {
            // Tell garbage collector to eat this now.
            R_ReleaseObject(tracer[type]);
            tracer[type] = NULL;
        }
    }

  private:
    SEXP tracer[Type::RIR_TRACE_NUM_OF];
};
}

#else

extern SEXP tracing_get(enum Type);

#endif

#endif // RIR_TRACING_H
