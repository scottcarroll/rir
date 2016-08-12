#ifndef RIR_FRAMEWORK_H
#define RIR_FRAMEWORK_H

#include <deque>
#include "ir/BC.h"
#include "ir/CodeEditor.h"

namespace rir {

/** Dispatcher prototype.

  A dispatcher must determine two things:

  1) what code from the receiver will be executed based on the current status
  2) how far should the cursor advance

  The receiver is not part of the Dispatcher class because it heavily depends on the dispatch method used and would therefore require the use of templates.

 */
class Dispatcher {
public:
    virtual ~Dispatcher() {
    }

    /** Dispatches on the given cursor, and returns true if the dispatch was successful, false if not.

      TODO perhaps the dispatch success is overkill.
     */
    bool dispatch(CodeEditor::Cursor & cursor) {
        success_ = true;
        doDispatch(cursor);
        return success_;
    }

protected:
    Dispatcher() = default;

    /** Called by actual dispatchers when they want to notify the dispatching that it has failed.

      When this method is called from a dispatched routine, the dispatch() method will the return false.
     */
    void fail() {
        success_ = false;
    }

    /** Actual dispatch code.

      Must be implemented in children.
     */
    virtual void doDispatch(CodeEditor::Cursor & cursor) = 0;

    bool success_;
};

/** The driver defines the pattern of which instructions and when will the dispatcher see.

  Paired with a dispatcher that performs the dispatch to receiver methods, the driver is responsible for advancing the cursor according to the driving implementation (i.e. linear, forward analysis, etc. ).

  The very non-restrictive API of dispatchers and drivers is by design and not every possible combination of dispatcher and driver is expected to work.
 */
class Driver {
public:
protected:
    /** The actual driver method.

      Should be overriden in each driver type and called from the run() method.
     */
    virtual void doRun(CodeEditor & code, Dispatcher & dispatcher) = 0;
};

}
#endif
