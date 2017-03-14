//
// Created by ksiek on 14.03.17.
//

#include "Tracing.h"

using namespace rir;

extern "C" SEXP tracing_get(Tracing::Type type) {
    switch (type) {
        case Tracing::Type::RIR_TRACE_CALL:
            return Tracing::instance().getTracer(Tracing::Type::RIR_TRACE_CALL);
        default:
            assert(false);
    }
    return nullptr;
}