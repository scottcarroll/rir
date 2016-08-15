#ifndef RIR_OPTIMIZER_CP_H
#define RIR_OPTIMIZER_CP_H

#include "code/analysis.h"
#include "code/InstructionVisitor.h"
#include "interpreter/interp_context.h"

namespace rir {

/** Constant Propagation Abstract Value.
 */
class CP_Value {
  public:
    static CP_Value const top;
    static CP_Value const bottom;

    CP_Value(SEXP from) : value_(from) {}

    CP_Value(CP_Value const& other) = default;

    CP_Value& operator=(SEXP what) {
        value_ = what;
        return *this;
    }

    CP_Value& operator=(CP_Value const& other) = default;

    bool operator==(CP_Value const& other) const {
        return value_ == other.value_;
    }

    bool operator!=(CP_Value const& other) const {
        return value_ != other.value_;
    }

    SEXP value() const {
        assert(value_ != bottom_ and value_ != top_);
        return value_;
    }

    bool isConst() const { return value_ != bottom_ and value_ != top_; }

    bool mergeWith(CP_Value const& other) {
        if (*this == top)
            return false;
        if (*this == other)
            return false;
        if (*this == bottom)
            value_ = other.value_;
        else
            value_ = top_;
        return true;
    }

    void print() {
        if (value_ == top_)
            Rprintf("T");
        else if (value_ == bottom_)
            Rprintf("B");
        else
            Rf_PrintValue(value_);
    }

  protected:
    constexpr static SEXP const bottom_ = nullptr;
    constexpr static SEXP const top_ = (SEXP)(1);

    SEXP value_;

};

class ConstantPropagation : public ForwardAnalysis<AbstractState<CP_Value>>,
                            public InstructionVisitor::Receiver {
  public:
    typedef CP_Value Value;
    ConstantPropagation() : ForwardAnalysis() {
        dispatcher_ = new InstructionVisitor(*this);
    }

    ~ConstantPropagation() { delete dispatcher_; }

    void invalidate() override {
        ForwardAnalysis::invalidate();
        delete initialState_;
        initialState_ = nullptr;
    }

  protected:
    void doAnalyze(CodeEditor& code) override {
        setInitialState(new AbstractState<CP_Value>());
        ForwardAnalysis::doAnalyze(code);
    }

    void push_(CodeEditor::Cursor ins) override {
        current().push(ins->immediateConst());
    }

    void ldvar_(CodeEditor::Cursor ins) override {
        current().push(current().env().find(ins->immediateConst()));
    }

    void stvar_(CodeEditor::Cursor ins) override {
        current()[ins->immediateConst()] = current().pop();
    }

    /** All other instructions, don't care for now.
     */
    void any(CodeEditor::Cursor ins) override {
        // pop as many as we need, push as many tops as we need
        current().pop(ins->popCount());
        for (size_t i = 0, e = ins->pushCount(); i != e; ++i)
            current().push(Value::top);
    }

};
}
#endif
