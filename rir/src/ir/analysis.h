#include <deque>

#include "BC.h"
#include "CodeEditor.h"


namespace rir {

/** Each instruction has its representation.

  These small classes are byte for byte compatible with the instruction layout in the CodeEditor objects and can be used for simple querying of the arguments.
 */
namespace bc {
    class Any {
    public:
        BC_t opcode;
    };

    class Push : public Any {
    public:
    };

    class LdFun : public Any {

    };
    class LdDDvar : public Any {

    };
    class LdVar : public Any {

    };
    class Call : public Any {

    };
    class Promise : public Any {

    };
    class Close : public Any {

    };
    class Ret : public Any {

    };
    class Force : public Any {

    };
    class Pop : public Any {

    };
    class PushArg : public Any {

    };
    class AsAst : public Any {

    };

    class StVar : public Any {

    };
    class AsBool : public Any {

    };
    class BrTrue : public Any {
    };

    class BrFalse : public Any {
    };

    class Br : public Any {
    };
    class Invisible : public Any {

    };

    class LtI : public Any {

    };

    class EqI : public Any {

    };

    class PushI : public Any {

    };
    class DupI : public Any {

    };
    class Dup : public Any {

    };
    class Add : public Any {

    };

    class Sub : public Any {

    };
    class Lt : public Any {

    };
    class IsSpecial : public Any {

    };

    class IsFun : public Any {

    };









} // namespace rir::bc

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

    /** Dispatches on the given cursor, advances the cursor and returns true if the dispatch was successful, false if not.

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


/** Dispatches based on instruction type only.

  Basically a instruction type visitor implementation.
 */
class InstructionVisitor : public Dispatcher {
public:

    /** Receiver for InstructionVisitor dispatcher.

      TODO The receiver should define a simple yet useful hierarchy of functions handling the respective instructions, or their groups, based on their opcodess.
     */
    class Receiver {
    public:
        virtual ~Receiver() {
        }
    protected:
        friend class InstructionVisitor;

        virtual void any(CodeEditor::Cursor & ins) {
        }
/* Virtual function for each instruction, all calling to any.
 */

#define DEF_INSTR(name, ...) virtual void name(CodeEditor::Cursor & ins) { any(ins); }
#include "insns.h"

    };

    InstructionVisitor(Receiver & receiver):
        receiver_(receiver) {
    }

protected:

    void doDispatch(CodeEditor::Cursor & cursor) override {
        BC cur = *cursor;
        switch (cur.bc) {

/* Dispatch on instruction types for all instructions.
 */
#define DEF_INSTR(name, ...) case BC_t::name: receiver_.name(cursor); break;
#include "insns.h"

            default:
                // dispatcher failure because of unknown instruction
                fail();
        }
        // move to the next instruction
        ++cursor;
    }

private:

    Receiver & receiver_;
};



/** The driver defines the pattern of which instructions and when will the dispatcher see.
 */
class Driver {
public:
    /** Runs the driver on given code object.

      This is the public API and should be redefined in the final driver, when the main dispatcher becomes known.
     */
    virtual void run(CodeEditor & code) = 0;

protected:

    Driver() = default;

    /** The actual driver method.

      Should be overriden in each driver type and called from the run() method.
     */
    virtual void doRun(CodeEditor & code, Dispatcher & dispatcher) = 0;

};

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

/** Represents a state for forward and backward drivers to determine whether a fixpoint has been reached, or not.
 */
class State {
public:

    /** Just to be safe.
     */
    virtual ~State() {
    }

    /** Creates a deep copy of the state.
     */
    virtual State * clone() = 0;

    /** Merges the other state information into this state.

      Returns true if the state has changed, false otherwise.
     */
    virtual bool mergeWith(State const * other) = 0;
};


// ===============================================================================================

/** The control flow dispatcher is an example of non-instruction based dispatcher and receiver.

  Here we do not care about the instructions as much as about the semantics, i.e. whether the execution terminates, or if we have conditional or unconditional jump (and where to), or if we have arrived at a label.
 */
class ControlFlowDispatcher : public Dispatcher {
public:

    class Receiver {
    public:
        virtual ~Receiver() {
        }
    protected:
        friend class ControlFlowDispatcher;

        virtual void conditionalJump(CodeEditor::Cursor target) = 0;

        virtual void unconditionalJump(CodeEditor::Cursor target) = 0;

        virtual void termination(CodeEditor::Cursor const & at) = 0;

        virtual void label(CodeEditor::Cursor const & at) = 0;
    };

    ControlFlowDispatcher(Receiver & receiver):
        receiver_(receiver) {
    }

protected:

    void doDispatch(CodeEditor::Cursor & cursor) override {
        BC cur = *cursor;
        switch (cur.bc) {
            case BC_t::ret_:
                receiver_.termination(cursor);
                break;
            case BC_t::label:
                receiver_.label(cursor);
                break;
            case BC_t::brtrue_:
            case BC_t::brfalse_:
                receiver_.conditionalJump(cursor.editor().label(cur.immediate.offset));
                break;
            case BC_t::br_:
                receiver_.unconditionalJump(cursor.editor().label(cur.immediate.offset));
                break;
            default:
                // dispatcher failure because of non-cf instruction
                fail();
        }
        // let's advance the cursor so that the dispatcher is generally useful
        ++cursor;
    }

private:
    Receiver & receiver_;
};

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

        void termination(CodeEditor::Cursor const & at) override;

        void label(CodeEditor::Cursor const & at) override;

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
    void free() {
        assert (q_.empty() and "You are not supposed to call free() while the driver is active");
        delete currentState_;
        for (State * s : mergepoints_)
            delete s;
        mergepoints_.clear();
    }

    void doRun(CodeEditor & code, Dispatcher & dispatcher) override {
        free();
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
                // keep backup of the cursor for cf operations
                CodeEditor::Cursor cx = c;
                // dispatch proper
                dispatcher.dispatch(c);
                // dispatch based on control flow
                dispatcher_.dispatch(cx);
            }
            // deletes the current state so that we do not leak
            delete currentState_;
        }
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

    State * storedState(CodeEditor::Cursor const & c) {
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

private:

    CFReceiver receiver_;
    ControlFlowDispatcher dispatcher_;

    std::deque<Item> q_;
    State * initialState_;
    State * currentState_;

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
inline void ForwardDriver::CFReceiver::termination(CodeEditor::Cursor const & at) {
    driver_.terminateCurrentExecution();
    // current state will be deleted by the driver
}

/** A label is basically a fixpoint check.
 */
inline void ForwardDriver::CFReceiver::label(CodeEditor::Cursor const & at) {
    driver_.checkFixpoint(at, driver_.currentState_);
}







/** A simple demonstration of the dispatching, a printer.

  As long as we need only single dispatcher, single driver and single receiver, they can all be parents of the class as they are in this simple example.

*/

class Printer : public InstructionVisitor::Receiver, LinearDriver {
public:
    Printer():
        dispatcher_(*this) {

    }

    void run(CodeEditor & code) {
        pc_ = 0;
        doRun(code, dispatcher_);
        for (size_t i = 0, e = code.numPromises(); i != e; ++i) {
            Rprintf("\n");
            printOffset();
            Rprintf("promise %i:\n", i);
            offset_ += 4;
            run(code.promise(i));
            offset_ -= 4;
        }
    }

protected:

    /** Some silly printer stuff
     */
    void any(CodeEditor::Cursor & ins) override {
        if (ins.hasAst()) {
            printOffset();
            Rprintf("          # ");
            Rf_PrintValue(ins.ast());
        }
        printOffset();
        Rprintf(" %5x ", pc_);
        BC bc = *ins;
        bc.print();
        pc_ += bc.size();
    }

private:

    // TODO slow & ugly
    void printOffset() {
        for (size_t i = 0; i != offset_; ++i)
            Rprintf(" ");
    }

    InstructionVisitor dispatcher_;
    size_t pc_ = 0;
    size_t offset_ = 0;
};

}
