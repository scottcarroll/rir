#ifndef RIR_FORWARD_DRIVER_H
#define RIR_FORWARD_DRIVER_H

#include "framework.h"
#include "ControlFlowDispatcher.h"
#include "state.h"

namespace rir {


/** Forward analysis pass driver.

  For the forward analysis pass, we need the jumpoints analysis results. i.e. how am I going to preserve analysis results inside

 */
class ForwardDriver : public Driver {

protected:

    /** The control flow receiver actually does the driving.
     */
    class CFReceiver : public ControlFlowDispatcher::Receiver {
    public:
        CFReceiver(ForwardDriver & driver):
            driver_(driver) {
        }
    protected:
        void conditionalJump(CodeEditor::Cursor target) override;

        void unconditionalJump(CodeEditor::Cursor target) override;

        void termination(CodeEditor::Cursor at) override;

        void label(CodeEditor::Cursor at) override;

    private:
        // a bit wasteful, but safe. Another option would be to make the forward driver itself implement the ControlFlow::Receiver interface, but that would mean the ControlFlow receiver would be disabled from future considerations.
        ForwardDriver & driver_;
    };

    ForwardDriver(State * initialState):
        receiver_(*this),
        dispatcher_(receiver_),
        initialState_(initialState) {
    }

    /** Frees memory allocated by the driver after the run.

      This deletes the current state (if any) and all mergepoints.
     */
    void cleanup() {
        assert (q_.empty() and "You are not supposed to call free() while the driver is active");
        delete currentState_;
        for (State * s : mergepoints_)
            delete s;
        mergepoints_.clear();
    }

    void doRun(CodeEditor & code, Dispatcher & dispatcher) override {
        cleanup();
        q_.clear();
        q_.push_back(Item(code.getCursor(), initialState_->clone()));
        while (not q_.empty()) {
            terminate_ = false;
            // get stuff from the queue
            CodeEditor::Cursor c = q_.front().cursor;
            State * incommingState = q_.front().state;
            q_.pop_front();
            // no need to check fixpoint when we start the iteration - the first label instruction will do that for us
            while (not terminate_) {
                // dispatch proper
                dispatcher.dispatch(c);
                // dispatch based on control flow
                dispatcher_.dispatch(c);
                // move the cursor
                ++c;
            }
            // deletes the current state so that we do not leak
            delete currentState_;
        }
    }

    virtual ~ForwardDriver() {
        cleanup();
        delete initialState_;
    }

protected:

    friend class CFReceiver;

    struct Item {
        CodeEditor::Cursor cursor;
        State * state;
        Item(CodeEditor::Cursor const & cursor, State * state):
            cursor(cursor),
            state(state) {
        }
    };

    void storeState(CodeEditor::Cursor const & c, State * state) {
        BC ins = *c;
        assert (ins.bc == BC_t::label);
        assert (mergepoints_[ins.immediate.offset] == nullptr);
        mergepoints_[ins.immediate.offset] = state;
    }

    State * storedState(CodeEditor::Cursor const & c) const {
        BC ins = *c;
        assert (ins.bc == BC_t::label);
        return mergepoints_[ins.immediate.offset];
    }

    /** Terminates current execution.

      Useful when the analysis, or the driver itself determine that further execution is not necessary.
     */
    void terminateCurrentExecution() {
        terminate_ = true;
    }

    void checkFixpoint(CodeEditor::Cursor const & current, State * incomming) {

        State * stored = storedState(current);
        if (stored == nullptr) {
            stored = incomming->clone();
            storeState(current, stored);
            currentState_ = incomming;
        } else {
            if (not stored->mergeWith(incomming)) {
                terminateCurrentExecution();
            } else {
                currentState_ = stored->clone();
            }
            delete incomming;
        }
    }

protected:
    /** Used in analyses for retrieval caching. */
    mutable State * currentState_;
    State * initialState_;


private:

    CFReceiver receiver_;
    ControlFlowDispatcher dispatcher_;

    std::deque<Item> q_;

    /** If true, terminates the execution of current queue path.
     */
    bool terminate_;


    /** An array of stored states at mergepoits.

      Note that we only store the beginning of the function as mergepoint if it also is a label instruction as otherwise there are no jumps to it, and therefore no merges.
     */
    std::vector<State *> mergepoints_;

};

/** Conditional labale queues the target cursor with clone of current state as incomming and continues the execution.
 */
inline void ForwardDriver::CFReceiver::conditionalJump(CodeEditor::Cursor target) {
    driver_.q_.push_back(Item(target, driver_.currentState_->clone()));
}

/** Unconditional label queues the target cursor and current state, then terminates current execution.

  There is no need to delete current state as it will be reused in the queued branch.
 */
inline void ForwardDriver::CFReceiver::unconditionalJump(CodeEditor::Cursor target) {
    driver_.q_.push_back(Item(target, driver_.currentState_));
    driver_.terminateCurrentExecution();
    // because we continue with current state in the queue
    driver_.currentState_ = nullptr;
}

/** When we see a terminator, just terminate the current execution.
 */
inline void ForwardDriver::CFReceiver::termination(CodeEditor::Cursor at) {
    driver_.terminateCurrentExecution();
    // current state will be deleted by the driver
}

/** A label is basically a fixpoint check.
 */
inline void ForwardDriver::CFReceiver::label(CodeEditor::Cursor at) {
    driver_.checkFixpoint(at, driver_.currentState_);
}



}
#endif
