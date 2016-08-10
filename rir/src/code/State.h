#ifndef RIR_STATE_H
#define RIR_STATE_H

#include <map>

namespace rir {

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



class AbstractStack {

protected:


};



class AbstractEnvironment {
public:
protected:

};



class AbstractState : public State {
private:




};




}
#endif
