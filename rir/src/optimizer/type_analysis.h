#ifndef RIR_OPTIMIZER_TYPE_H
#define RIR_OPTIMIZER_TYPE_H



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
        // TODO fill this in


        return result;
    }

private:
    Type type_;
    ElementType eType_;
    Length length_;
    Exists exists_;
};




}


#endif
