#ifndef RIR_STATE_H
#define RIR_STATE_H

#include <map>
#include <deque>
#include <cassert>

#include "R/r.h"

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
    virtual State * clone() const = 0;

    /** Merges the other state information into this state.

      Returns true if the state has changed, false otherwise.
     */
    virtual bool mergeWith(State const * other) = 0;
};



/** This is what we want from AVALUE
 */
class AVALUE {

public:
    AVALUE() = delete;

    AVALUE(AVALUE const &) = default;

    static AVALUE const bottom;
    static AVALUE const top;

    bool mergeWith(AVALUE const & other) {
        return false;
    }


};



/** Stack model.

  Model of an abstract stack is relatively easy - since for correct code, stack depth at any mergepoints must be constant, so stack merging is only a merge of the stack's values.
 */
class AbstractStack : public State {
public:
    AbstractStack() = default;
    AbstractStack(AbstractStack const &) = default;

    /** Clone is just a copy constructor.
     */
    AbstractStack * clone() const override {
        return new AbstractStack(*this);
    }

    /** Merge with delegates to properly typed function.
     */
    bool mergeWith(State const * other) override {
        assert(dynamic_cast<AbstractStack const*>(other) != nullptr);
        return mergeWith(*dynamic_cast<AbstractStack const *>(other));
    }

    /** Merges the other stack into itself.

      Returns true if any of stack values changed during the process.
     */
    bool mergeWith(AbstractStack const & other) {
        assert(depth() == other.depth() and "At merge stacks must have the same height");
        bool result = false;
        for (size_t i = 0, e = stack_.size(); i != e; ++i)
            result = stack_[i].mergeWith(other.stack_[i]) or result;
        return result;
    }

    AVALUE pop() {
        AVALUE result = stack_.front();
        stack_.pop_front();
        return result;
    }

    void pop(size_t num) {
        assert(stack_.size() >= num);
        for (size_t i = 0; i != num; ++i)
            stack_.pop_front();
    }

    void push(AVALUE value) {
        stack_.push_front(value);
    }

    size_t depth() const {
        return stack_.size();
    }

    bool empty() const {
        return stack_.size() == 0;
    }

    AVALUE const & operator[](size_t idx) const {
        assert(idx < stack_.size());
        return stack_[idx];
    }

    AVALUE & operator[](size_t idx) {
        assert(idx < stack_.size());
        return stack_[idx];
    }

protected:
    std::deque<AVALUE> stack_;
};


class AbstractEnvironment : public State {
public:
    AbstractEnvironment():
        parent_(nullptr) {
    }

    AbstractEnvironment(AbstractEnvironment const &from ):
        parent_(from.parent_ == nullptr ? nullptr : from.parent_->clone()),
        env_(from.env_) {
    }

    AbstractEnvironment * clone() const override {
        return new AbstractEnvironment(*this);
    }

    bool mergeWith(State const * other) override {
        assert(dynamic_cast<AbstractEnvironment const*>(other) != nullptr);
        return mergeWith(*dynamic_cast<AbstractEnvironment const *>(other));
    }

    /** Merges the other environment into this one.

      TODO Note that merge also merges parent environments - this might require more thinking.
     */
    bool mergeWith(AbstractEnvironment const & other) {
        bool result = false;

        for (auto i = other.env_.begin(), e = other.env_.end(); i != e; ++i) {
            auto own = env_.find(i->first);
            if (own == env_.end()) {
                // if there is a variable in other that is not in us, just copy it and mark as changed
                env_.insert(*i);
                //env_[i->first] = i->second;
                result = true;
            } else {
                // otherwise try merging it with our variable
                result = own->second.mergeWith(i->second) or result;
            }
            // and we do not care about variables that we have and other does not (that means they will be bottom in his, and therefore our values will not change
        }

        // merge parents

        if (parent_ == nullptr) {
            if (other.parent_ != nullptr) {
                parent_ = new AbstractEnvironment(*other.parent_);
                result = true;
            }
        } else if (other.parent_ != nullptr) {
            result = parent_->mergeWith(*(other.parent_)) or result;
        }
        return result;
    }

    // TODO is this what we want wrt parents?
    bool empty() const {
        return env_.empty();
    }

    bool has(SEXP name) const {
        return env_.find(name) != env_.end();
    }

    AVALUE const & operator[](SEXP name) const {
        auto i = env_.find(name);
        if (i == env_.end())
            return AVALUE::bottom;
        else
            return i->second;
    }

    AVALUE & operator[](SEXP name) {
        auto i = env_.find(name);
        if (i == env_.end()) {
            // so that we do not demand default constructor on values
            env_.insert(std::pair<SEXP, AVALUE>(name, AVALUE::bottom));
            i = env_.find(name);
            return i->second;
        } else {
            return i->second;
        }
    }

    bool hasParent() const {
        return parent_ != nullptr;
    }

    AbstractEnvironment & parent() {
        assert(parent_ != nullptr);
        return * parent_;
    }


    // TODO implement find to look through parent environmets as well?

protected:

    AbstractEnvironment * parent_ = nullptr;

    std::map<SEXP, AVALUE> env_;

};


/** This could be done with multiple virtual inheritance, but the composition is simpler, and perhaps even cleaner, albeit more lengthy.
 */

class AbstractState : public State {
public:
    AbstractState() = default;
    AbstractState(AbstractState const &) = default;

    AbstractState * clone() const override {
            return new AbstractState(*this);
    }

    bool mergeWith(State const * other) override {
        assert(dynamic_cast<AbstractState const*>(other) != nullptr);
        return mergeWith(*dynamic_cast<AbstractState const *>(other));
    }

    AbstractStack const & stack() const {
        return stack_;
    }

    AbstractStack & stack() {
        return stack_;
    }

    AbstractEnvironment const & env() const {
        return env_;
    }

    AbstractEnvironment & env() {
        return env_;
    }

    bool mergeWith(AbstractState const & other) {
        bool result = false;
        result = stack_.mergeWith(other.stack_) or result;
        result = env_.mergeWith(other.env_) or result;
        return result;
    }

    AVALUE pop() {
        return stack_.pop();
    }

    void pop(size_t num) {
        stack_.pop(num);
    }

    void push(AVALUE value) {
        stack_.push(value);
    }

    AVALUE const & operator[](size_t index) const {
        return stack_[index];
    }

    AVALUE & operator[](size_t index) {
        return stack_[index];
    }

    AVALUE const & operator[](SEXP name) const {
        return env_[name];
    }

    AVALUE & operator[](SEXP name) {
        return env_[name];
    }

protected:
    AbstractStack stack_;
    AbstractEnvironment env_;
};




}
#endif
