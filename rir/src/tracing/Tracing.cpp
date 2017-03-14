//
// Created by ksiek on 14.03.17.
//

#include "Tracing.h"

using namespace rir;

extern "C" SEXP tracing_get(Tracing::Type type) {
    if (type >= Tracing::Type::RIR_TRACE_NUM_OF)
        assert(false);
    return Tracing::instance().getTracer(type);
}