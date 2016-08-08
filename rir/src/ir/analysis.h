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

        virtual void any(BC ins) {
        }

        virtual void push(BC ins) {
            any(ins);
        }

        virtual void ldfun(BC ins) {
            any(ins);
        }

        virtual void ldddvar(BC ins) {
            any(ins);
        }

        virtual void ldvar(BC ins) {
            any(ins);
        }

        virtual void call(BC ins) {
            any(ins);
        }

        virtual void promise(BC ins) {
            any(ins);
        }

        virtual void close(BC ins) {
            any(ins);
        }

        virtual void ret(BC ins) {
            any(ins);
        }

        virtual void force(BC ins) {
            any(ins);
        }

        virtual void pop(BC ins) {
            any(ins);
        }

        virtual void pusharg(BC ins) {
            any(ins);
        }

        virtual void asast(BC ins) {
            any(ins);
        }

        virtual void stvar(BC ins) {
            any(ins);
        }

        virtual void asbool(BC ins) {
            any(ins);
        }

        virtual void brtrue(BC ins) {
            any(ins);
        }

        virtual void brfalse(BC ins) {
            any(ins);
        }

        virtual void br(BC ins) {
            any(ins);
        }

        virtual void invisible(BC ins) {
            any(ins);
        }

        virtual void lti(BC ins) {
            any(ins);
        }

        virtual void eqi(BC ins) {
            any(ins);
        }

        virtual void pushi(BC ins) {
            any(ins);
        }

        virtual void dupi(BC ins) {
            any(ins);
        }

        virtual void dup(BC ins) {
            any(ins);
        }

        virtual void add(BC ins) {
            any(ins);
        }

        virtual void sub(BC ins) {
            any(ins);
        }

        virtual void lt(BC ins) {
            any(ins);
        }

        virtual void isspecial(BC ins) {
            any(ins);
        }

        virtual void isfun(BC ins) {
            any(ins);
        }

        virtual void inci(BC ins) {
            any(ins);
        }

        virtual void push_argi(BC ins) {
            any(ins);
        }
    };

    InstructionVisitor(Receiver & receiver):
        receiver_(receiver) {
    }

protected:

    void doDispatch(CodeEditor::Cursor & cursor) override {
        BC cur = *cursor;
        switch (cur.bc) {
            case BC_t::push_:
                receiver_.push(cur);
                break;
            case BC_t::ldfun_:
                receiver_.ldfun(cur);
                break;
            case BC_t::ldddvar_:
                receiver_.ldddvar(cur);
                break;
            case BC_t::ldvar_:
                receiver_.ldvar(cur);
                break;
            case BC_t::call_:
                receiver_.call(cur);
                break;
            case BC_t::promise_:
                receiver_.promise(cur);
                break;
            case BC_t::close_:
                receiver_.close(cur);
                break;
            case BC_t::ret_:
                receiver_.ret(cur);
                break;
            case BC_t::force_:
                receiver_.force(cur);
                break;
            case BC_t::pop_:
                receiver_.pop(cur);
                break;
            case BC_t::pusharg_:
                receiver_.pusharg(cur);
                break;
            case BC_t::asast_:
                receiver_.asast(cur);
                break;
            case BC_t::stvar_:
                receiver_.stvar(cur);
                break;
            case BC_t::asbool_:
                receiver_.asbool(cur);
                break;
            case BC_t::brtrue_:
                receiver_.brtrue(cur);
                break;
            case BC_t::brfalse_:
                receiver_.brfalse(cur);
                break;
            case BC_t::br_:
                receiver_.br(cur);
                break;
            case BC_t::invisible_:
                receiver_.invisible(cur);
                break;
            case BC_t::lti_:
                receiver_.lti(cur);
                break;
            case BC_t::eqi_:
                receiver_.eqi(cur);
                break;
            case BC_t::pushi_:
                receiver_.pushi(cur);
                break;
            case BC_t::dupi_:
                receiver_.dupi(cur);
                break;
            case BC_t::dup_:
                receiver_.dup(cur);
                break;
            case BC_t::add_:
                receiver_.add(cur);
                break;
            case BC_t::sub_:
                receiver_.sub(cur);
                break;
            case BC_t::lt_:
                receiver_.lt(cur);
                break;
            case BC_t::isspecial_:
                receiver_.isspecial(cur);
                break;
            case BC_t::inci_:
                receiver_.inci(cur);
                break;
            case BC_t::push_argi_:
                receiver_.push_argi(cur);
                break;
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

/** A simple dispatcher for control flow related instructions only.

 */
class ControlFlowDispatcher : public Dispatcher {
public:
    /** Control flow receiver is only concerned with instructions that may alter the control flow.

      All of them are abstract so that they must be redefined in actual implementations.
     */
    class Receiver {
    public:
        virtual ~Receiver() {
        }
    protected:
        friend class ControlFlowDispatcher;

        virtual void ret(BC ins) = 0;

        virtual void brtrue(BC ins) = 0;

        virtual void brfalse(BC ins) = 0;

        virtual void br(BC ins) = 0;

        virtual void brobj(BC ins) = 0;
    };

    ControlFlowDispatcher(Receiver & receiver):
        receiver_(receiver) {
    }
protected:

    void doDispatch(CodeEditor::Cursor & cursor) override {
        BC cur = *cursor;
        switch (cur.bc) {
            case BC_t::ret_:
                receiver_.ret(cur);
                break;
            case BC_t::brtrue_:
                receiver_.brtrue(cur);
                break;
            case BC_t::brfalse_:
                receiver_.brfalse(cur);
                break;
            case BC_t::br_:
                receiver_.br(cur);
                break;
            case BC_t::brobj_:
                receiver_.brobj(cur);
                break;
            default:
                // dispatcher failure because of non-cf instruction
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



/** Forward analysis pass driver.

  For the forward analysis pass, we need the jumpoints analysis results. i.e. how am I going to preserve analysis results inside

 */
class ForwardDriver : public Driver {

protected:

    ForwardDriver(State * initialiState):
        initialState_(initialState) {
    }

    class CFReceiver;





    void doRun(CodeEditor & code, Dispatcher & dispatcher) override {
        q_.clear();
        q_.push_back(Item(code.getCursor(), initialState_->clone()));
        while (not q_.empty()) {
            // get the current
            CodeEditor::Cursor c = q_.front().cursor;
            current_ = q_.front().state;
            q_.pop_front();
            // check the incomming state.
            State * stored = storedState(c);
            // we haven't seen the state yet, use incomming as stored
            if (stored == nullptr) {

            }

            // now do the dispatching for the current cursor.





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

    }

    State * storedState(CodeEditor::Cursor const & c) {
        // TODO
        return nullptr;
    }

    ControlFlowDispatcher cfDispatcher_;

    std::deque<Item> q_;
    State * initialState_;
    State * current_;
};


/** Does the scheduling for the forward driver.
 */
class ForwardDriver::CFReceiver : public ControlFlowDispatcher::Receiver {
public:
    CFReceiver(ForwardDriver & driver):
        driver_(driver) {
    }

protected:
    /** Return does nothing as it simply terminates the current execution.
     */
    virtual void ret(BC ins) override {
        // noop
    }

    virtual void brtrue(BC ins) override {
        // enque the next instruction with

    }

    virtual void brfalse(BC ins) override {

    }

    virtual void br(BC ins) override {
        // create a cursor for the target jump and enque it with current state
    }

    virtual void brobj(BC ins) override {

    }


private:
    ForwardDriver & driver_;


};





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
    }

protected:

    /** Some silly printer stuff
     */
    void any(BC ins) override {
        std::cout << pc_ << std::endl;
        // TODO print the size and so on
        pc_ += ins.size();
    }

    void ldvar(BC ins) override {
        any(ins);
        std::cout << "ldvar" << std::endl;
    }

    void ldfun(BC ins) override {
        any(ins);
        std::cout << "ldfun" << std::endl;
    }


private:
    InstructionVisitor dispatcher_;
    size_t pc_ = 0;
};

}
