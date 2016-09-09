#pragma once

namespace rir {

/** Abstract value.

  Most likely, this class will not exist and it will be a template, but for now I am using it as a demonstrator of what functionality we require from a value.

  Also, if the assumption diferentiation will be merged into abstract values, we might actually need this class, as a CRTP base for all abstract values?

 */
class AVALUE {
public:
    /** Default constructor creates bottom.
     */
    AVALUE();

    /** Copy constructor
     */
    AVALUE(AVALUE const &) = default;

    /** Equality comparison
     */
    bool operator == (AVALUE const &) const;

    /** Inequality comparison
     */
    bool operator != (AVALUE const &) const;

    /** Merge with other value (returns true if changed)
     */
    bool merge(AVALUE const & other);

    /** Join of two abstract values (basically the result of a merge)
     */
    AVALUE upper(AVALUE const & other) const;

    /** Meet of two abstract values (what assumption and analyzed value together create)
     */
    AVALUE lower(AVALUE const & other) const;

    /** For the value, returns the part of it that can be closure.

      There does not have to be any actual ordering between the returned value and the value itself, however the purpose of this function is to return an abstract value that can be used for value merging in closure lookup.

      If `bottom` is returned, it means that the value is definitely *not* a closure.

      `top` means that nothing is known about the value being closure or not and is therefore the default behavior.

      If the same value as itself is returned and it is not `top`, it means that the value is definitely a closure and the lookup may stop.

      If anything else is returned, then the meaning is that it may be a closure, and if it is, all closures it may hold are represented by the returned value.
     */
    AVALUE closurePart() const;


    /** Creates top abstract value. */
    static AVALUE top();

    /** Creates bottom abstract value. */
    static AVALUE bottom();
};



}
