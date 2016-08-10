#ifndef RIR_LINEAR_DRIVER_H
#define RIR_LINEAR_DRIVER_H

#include "framework.h"

namespace rir {

/** A linear driver that just executes the given dispatcher over the entire code in the code editor starting from the first instruction to the last one.

  The dispatcher is allowed to skip instructions if it wishes to do so.
 */
class LinearDriver : public Driver {

protected:
    void doRun(CodeEditor & code, Dispatcher & dispatcher) override {
        // no need to iterate in the loop - the dispatcher should advance accordingly
        for (CodeEditor::Cursor i = code.getCursor(), e = code.getCursorAtEnd(); i != e; ) {
            dispatcher.dispatch(i);
        }
    }
};


}
#endif
