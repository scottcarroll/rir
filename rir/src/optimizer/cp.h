#ifndef RIR_OPTIMIZER_CP_H
#define RIR_OPTIMIZER_CP_H

#include "code/analysis.h"
#include "code/InstructionVisitor.h"

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

  protected:
    void doAnalyze(CodeEditor& code) override {
        setInitialState(new AbstractState<CP_Value>());
        ForwardAnalysis::doAnalyze(code);
    }
};
}
#endif
