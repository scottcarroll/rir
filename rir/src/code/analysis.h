#pragma once

#include "avalue.h"
#include "promise.h"
#include "execution.h"

namespace rir {


/** The analysis base class.

  Analysis handles the execution state. There are two execution states, `normal` and `maybe` and they affect how environments store their values.

 */
class Analysis {
public:

    /** Returns current execution state.
     */
    Execution & execution();

    /** Executes the given code in its environment using the provided execution mode and returns the result of that execution.

      This is the API for analysis to do its job.
     */
    AVALUE execute(Code * code, Environment env, Execution::Mode mode);

    /** A shorthand function that modifies the state of the analysis as if the worst had happened.
     */
    void executeUnknownCode();




};




}
