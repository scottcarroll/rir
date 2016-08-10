#ifndef RIR_INSTRUCTION_VISITOR_H
#define RIR_INSTRUCTION_VISITOR_H
#include "framework.h"

namespace rir {

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

        virtual void any(CodeEditor::Cursor ins) {
        }

        virtual void label(CodeEditor::Cursor ins) {
            any(ins);
        }

/* Virtual function for each instruction, all calling to any.
 */

#define DEF_INSTR(name, ...) virtual void name(CodeEditor::Cursor ins) { any(ins); }
#include "ir/insns.h"

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
#include "ir/insns.h"
            case BC_t::label:
                receiver_.label(cursor);
                break;
            default:
                // dispatcher failure because of unknown instruction
                fail();
        }
    }

private:

    Receiver & receiver_;
};


}
#endif
