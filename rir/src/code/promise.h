#pragma once

#include "environment.h"

namespace rir {

/** Contains the code to be analyzed (function granularity, where promise is a function too) and its properties.

  TODO perhaps they should not be just booleans, but some assumption types so that we can distinguish the degree of certainty.
 */
class Code {
public:
    bool hasSideeffects();
    bool evaluatesArguments();
};

/** Class representing a promise (or code in general).
 */
class Promise {
public:
    /** Returns the code to be evaluated. */
    Code * code();

    /** Returns the environment in which to evaluate the code.
     */
    Environment env();
};



}
