#ifndef RIR_PRINTER_H
#define RIR_PRINTER_H

#include "LinearDriver.h"
#include "InstructionVisitor.h"

namespace rir {

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

    /** Some silly printer stuff.
     */
    void any(CodeEditor::Cursor ins) override {
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

    void label(CodeEditor::Cursor ins) override {
        printOffset();
        Rprintf("Label %i:\n", ins->immediate.offset);
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
#endif
