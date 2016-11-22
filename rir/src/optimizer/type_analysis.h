#ifndef RIR_OPTIMIZER_TYPE_H
#define RIR_OPTIMIZER_TYPE_H

#include <algorithm>

#include "../ir/CodeEditor.h"
#include "../code/framework.h"
#include "../code/analysis.h"


namespace rir {


/** Our abstract value.

  In this example we are going to do a very simple type analysis for a rather limited set of R. THe limitations we make to R are as follows:

  - we assume there are no superassignments and weird environment things in the code (i.e. calling a function will not change anything in your local environment)
  - all promises are evaluated in the arguments order at the callsite
  - builtin functions (arithmetics, control flow, `c`) are never redefined
 */

class AType {
public:

    /** A lattice that tells us whether given variable lives in current frame.

      `yes` means the variable definitely exists in the frame
      `no` means the variable definitely does not exist in the frame
      `top` means the variable might be in the frame
     */
    enum class Exists {
        bottom,
        yes,
        no,
        top
    };

    /** Type of the variable, if it exists in the local frame.

      `vector` = a vector (we simplify and only concern ourselves with vectors of numbers, logicals and characters, everything else goes to `top`
      `closure` = a function
      `top` = anything (including possibly vectors and closures)

     */
    enum class Type {
        bottom,
        vector,
        closure,
        top
    };

    /** If the type is `vector`, this gives the type of its elements.

      `logical` = TRUE or FALSE
      `numeric` = any number (we are ignoring complex numbers)
      `character` = any string
      `top` = either logical, numeric or charcter - if it were something else, we do not consider it to be a vector

      */
    enum class ElementType {
        bottom,
        logical,
        numeric,
        character,
        top
    };

    /** Length of the vector, if it is a vector.

      -1 is bottom, -2 is top. Any other value means that the vector is guaranteed to be of the specified size.
     */

    static const int LENGTH_BOTTOM = -1;
    static const int LENGTH_TOP = -2;

    typedef int Length;


    AType(AType const & other)  = default;

    bool mergeWith(AType const & other) {
        bool result = false;
// TODO
        return result;
    }

    AType(Exists e = Exists::no):
        type(Type::bottom),
        eType(ElementType::bottom),
        length(LENGTH_BOTTOM),
        exists(e) {
    }

    static AType top() {
        return AType(Type::top, ElementType::top, LENGTH_TOP, Exists::yes);
    }

    static AType bottom() {
        return AType();
    }

    Type type;
    ElementType eType;
    Length length;
    Exists exists;

private:

    AType(Type t, ElementType et, Length l, Exists e):
        type(t),
        eType(et),
        length(l),
        exists(e) {
    }

};

class TypeAnalysis : public ForwardAnalysis<AbstractState<AType>>, public InstructionDispatcher::Receiver {
public:
    TypeAnalysis() :
        dispatcher_(*this) {

    }

protected:

    AbstractState<AType> * initialState() override {
        auto * result = new AbstractState<AType>();
        for (SEXP x : code_->arguments()) {
            (*result)[x].exists = AType::Exists::yes;
        }
        return result;
    }

    void ldvar_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        SEXP vName = bc.immediateConst();
        auto & x = current();
        x.stack().push(x.env()[vName]);
    }

    void stvar_(CodeEditor::Iterator ins) override {
        BC bc = *ins;
        SEXP vName = bc.immediateConst();
        auto & x = current();
        x.env()[vName] = x.stack().pop();
    }

    void add(CodeEditor::Iterator ins) {
        AType lhs = current().pop();
        AType rhs = current().pop();
        if (lhs.type == AType::Type::vector and rhs.type == AType::Type::vector) {
            int newLength;
            if (lhs.length == AType::LENGTH_TOP or rhs.length == AType::LENGTH_TOP)
                newLength = AType::LENGTH_TOP;
            else
                newLength = std::max(lhs.length, rhs.length);


        } else {
            current().push(AType::top());
        }


    }






    Dispatcher & dispatcher() override {
        return dispatcher_;
    }

private:
    InstructionDispatcher dispatcher_;


};



}


#endif
