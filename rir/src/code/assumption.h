#pragma once

#include "avalue.h"

#include "../R/r.h"

namespace rir {

/** Assumption about variable name and its value.

  The point of the assumption is to refine the value of a variable. The assumption is just a container and may hold the invalidation data at runtime.

  TODO assumptions & caching (see Environment)

  Actually it would be good to say that analysis never deals with assumptions and optimization "always deals with them" and use different API internally for the optimization based on the useAssumption (or actually sth like runCached) instead. The code would be a bit cleaner...

 */
class Assumption {
public:
    /** Returns name of the variable whose value we assume.
     */
    SEXP name();

    /** The actual value to be assumed.
     */
    AVALUE value();
};


}
