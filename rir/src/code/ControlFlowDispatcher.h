#ifndef RIR_CONTROL_FLOW_DISPATCHER_H
#define RIR_CONTROL_FLOW_DISPATCHER_H

#include "framework.h"

namespace rir {

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

        virtual void termination(CodeEditor::Cursor at) = 0;

        virtual void label(CodeEditor::Cursor at) = 0;
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
    }

private:
    Receiver & receiver_;
};

}
#endif
