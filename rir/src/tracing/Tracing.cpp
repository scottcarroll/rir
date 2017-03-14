//
// Created by ksiek on 14.03.17.
//

#include "Tracing.h"

using namespace rir;

extern "C" SEXP tracing_get(Tracing::Type type) {
    switch (type) {
    // TODO why is this necessary?
    case Tracing::Type::RIR_TRACE_CALL:
        return Tracing::instance().getTracer(Tracing::Type::RIR_TRACE_CALL);
    case Tracing::Type::RIR_TRACE_BUILTIN:
            return Tracing::instance().getTracer(Tracing::Type::RIR_TRACE_BUILTIN);
    case Tracing::Type::RIR_TRACE_PROMISE_EVAL:
        return Tracing::instance().getTracer(
            Tracing::Type::RIR_TRACE_PROMISE_EVAL);
    default:
        assert(false);
    }
    return nullptr;
}