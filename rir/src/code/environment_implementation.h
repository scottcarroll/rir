#pragma once

#include <set>
#include <map>


#include "analysis.h"
#include "promise.h"
#include "environment.h"

namespace rir {

/** A simple class that is used as a stop environment. For each variable lookup, returns top and executes the worst case promise.

  TODO Maybe this will become global environment and have slightly more complex semantic (i.e. it would retain values and also allow for storage.
 */
class TopStopEnvironment : public Environment::Implementation {

};

/** A simple class that is used as a stop environment.

  TODO the purpose of this is to differentiate possible read of uninitialized memory. Maybe this is not needed.
 */
class BottomStopEnvironment : public Environment::Implementation {


};




// TODO this guy also stores assumptions, and based on execution state should add them to the values retrieved
class HashMapEnvironment : public Environment::Implementation {
public:
    /** Finds variable in own hashmap and returns its value. If not found, searches in its parent.
     */
    AVALUE findVariable(SEXP name, Execution::Mode mode) {
        auto i = bindings_.find(name);
        if (i != bindings_.end()) {
            Binding & b(i->second);
            // if the binding is not a value, we must evaluate it first
            if (not b.isValue())
                b.evaluate(mode, analysis_);
            // if the binding is certain, we can return it
            if (b.storage == Binding::Storage::yes)
                return adjustAssumption(name, b.value);
            // otherwise we continue search in parent and merge
            AVALUE result = b.value;
            result.merge(parent_->findVariable(name, Execution::Mode::maybe));
            return adjustAssumption(name, result);
        }
        // not found in ourselves, try the parent
        return adjustAssumption(name, parent_->findVariable(name, mode));
    }

    /** Finds closure.
     */
    AVALUE findClosure(SEXP name, Execution::Mode mode) {
       auto i = bindings_.find(name);
       if (i != bindings_.end()) {
           Binding & b(i->second);
           // evaluate the value if not evaluated
           if (not b.isValue())
               b.evaluate(mode, analysis_);
           // get the closure part
           AVALUE closurePart = b.value.closurePart();
           // if the binding is certain and the closurePart is the value, which is not top, we know for sure we have found closure and can return it
           if (b.storage == Binding::Storage::yes and closurePart == b.value and b.value != AVALUE::top())
               return adjustAssumption(name, b.value);
           // otherwise if it might be closure, return merge of the closure part with further search in maybe mode
           assert(b.value != AVALUE::bottom() and "Binding with bottom abstract value should not exist");
           if (closurePart != AVALUE::bottom()) {
               closurePart.merge(parent_->findClosure(name, Execution::Mode::maybe));
               return adjustAssumption(name, closurePart);
           }
       }
       // we have either found nothing, or found something that is definitely not a closure, recurse
       return adjustAssumption(name, parent_->findClosure(name, mode));
    }

    /** Assigns given variable in specified execution mode.

      The assignment never propagates to parents as assignments in R go to specified environment. `maybe` assignments therefore does not mean that the store will maybe happen also elsewhere, but that the store maybe happens, and if it does, it definitely happens here.
     */
    void assign(SEXP name, AVALUE value, Execution::Mode mode) {
        auto i = bindings_.find(name);
        // if the binding does not exist, create one with appropriate mode
        if (i == bindings_.end()) {
            bindings_[name] = Binding(value, mode);
        } else {
            // for existing bindings, we either do update, or replace based on the mode
            if (mode == Execution::Mode::normal)
                i->second.setValue(value);
            else
                i->second.mergeValue(value);
        }
    }

    /** Super assignment in specified execution mode.

      If the binding is not found, super assignment recurses to parent environment preserving the mode. If a `yes` binding is found, the super assignment performs the assignment based on the execution mode and terminates.

      If `maybe` binding is found, the variable is `maybe` assigned and the super assignment recurses in the `maybe` mode regardless of its initial mode.

      TODO global environment should perform the write regardless of pre-existence of the binding.
     */
    void superassign(SEXP name, AVALUE value, Execution::Mode mode) {
        auto i = bindings_.find(name);
        if (i == bindings_.end()) {
            parent_->superassign(name, value, mode);
        } else if (i->second.storage == Binding::Storage::yes) {
            assign(name, value, mode);
        } else {
            assign(name, value, Execution::Mode::maybe);
            parent_->superassign(name, value, Execution::Mode::maybe);
        }
    }

    /** Returns the parent environment.
     */
    Implementation * parent() override {
        return parent_;
    }

    /** Destructor also detaches the parent.
     */
    ~HashMapEnvironment() override {
        if (parent_ != nullptr)
            parent_->detach();
    }

protected:
    // NOTE Perhaps this has to go to different class.
    struct Binding {

        /** A simple 3-way value. yes, no and maybe that determines whether a particular promise in the binding has been forced or not
         */
        enum class Forced {
            no,
            maybe,
            yes
        };


        /** Each binding may either be definitely present, or only maybe present (such as when control flow branch creates new binding.
         */
        enum class Storage {
            maybe,
            yes,
        };

        /** Abstract value of the binding. If the promises are only maybe evaluated, or only some are, this stores the join of already evaluated values.
         */
        AVALUE value;

        /** Storage type for the binding.
         */
        Storage storage;

        /** List of all promises that may be required t evaluate in order to get the value.
         */
        std::map<Promise *, Forced> promises;

        /** Merges two bindings together and returns true if changed.

          Merges the abstract values, the promises and the storage type. The abstract values are merged using their merge function. The storage type merges so that if both are `yes`, the result is `yes`, otherwise it is `maybe`.

          The promises are merged so that all promises from the other are added to own promises as they are. If both versions of the value have the same promise, their force status is merged so that two identical values produce the value and everything else is `maybe`:

               a\b  n m y
                n   n m m
                m   m m m
                y   m m y
         */
        bool merge(Binding const & other) {
            // merge values
            bool result = value.merge(other.value);
            // merge promises
            for (auto const & i : other.promises) {
                auto own = promises.find(i.first);
                if (own == promises.end()) {
                    promises[i.first] = i.second;
                    result = true;
                } else {
                    if (own->second != i.second and own->second != Forced::maybe) {
                        own->second = Forced::maybe;
                        result = true;
                    }
                }
            }
            // merge storage type
            if (storage == Storage::maybe and other.storage == Storage::yes) {
                storage = Storage::yes;
                result = true;
            }
            return result;
        }

        /** Returns true, if the binding is a simple value, or if all its promises have been evaluated.
         */
        bool isValue() const {
            for (auto i : promises)
                if (i.second != Forced::yes)
                    return false;
            return true;
        }

        /** Counts the number of unevaluated promises (or maybe evaluated).

          NOTE: practically we only need to know whether there is more than 1
         */
        unsigned unevaluatedPromises() {
            unsigned result = 0;
            for (auto i : promises)
                if (i.second != Forced::yes)
                    ++result;
            return result;
        }


        /** Sets the value of the binding, clearing any previous information.

          This is equivalent to a storage in `yes` mode. The value is set, promise information for the old value is cleared and the storage type is set to `yes`.
         */
        void setValue(AVALUE value) {
            value = value;
            storage = Storage::yes;
            promises.clear();
        }

        /** Merges the existing value with new one.

          This keeps all relevant information about the previously stored value and is equivalent to storage in the `maybe` mode.
         */
        void mergeValue(AVALUE value) {
            value.merge(value);
        }

        /** Evaluates the binding, running its promises, if necessary.

          Gets the list of promises that should be evaluated to refine the value and then evaluates them in respective execution mode:

          - if there are more than one promises that might be executed, mode changes to `maybe`
          - each promise is executed either in this mode, if it is not forced, or in `maybe` mode if it is maybe forced
          - the promise force status changes to maybe if it the mode is `maybe`, otherwise it changes to `yes`

         */
        void evaluate(Execution::Mode mode, Analysis & analysis) {
            if (unevaluatedPromises() > 1)
                mode = Execution::Mode::maybe;
            for (auto & i : promises) {
                Execution::Mode m = mode;
                switch (i.second) {
                    case Forced::maybe:
                        m = Execution::Mode::maybe;
                    case Forced::no:
                        value.merge(analysis.execute(i.first->code(),i.first->env(), m));
                        i.second = mode == Execution::Mode::maybe ? Forced::maybe : Forced::yes;
                        break;
                    case Forced::yes:
                        break;
                }
            }
        }


        Binding():
            value(AVALUE::bottom()),
            storage(Storage::yes) {
        }

        Binding(Binding const &) = default;

        Binding(AVALUE value, Execution::Mode mode):
            value(value),
            storage(mode == Execution::Mode::normal ? Storage::yes : Storage::maybe) {
        }

    };

    /** Adjusts the given abstract value with its name for an assumption that may be present in the environment.

      If assumptions are to be used, adjusts the returned value by using the meet of the assumed value and the computed one.

      TODO ideally, the abstract value would also keep track that its value is assumption based.
     */
    AVALUE adjustAssumption(SEXP name, AVALUE value) {
        if (analysis_.execution().useAssumptions()) {
            auto i = assumptions_.find(name);
            if (i != assumptions_.end())
                return value.lower(i->second->value());
        }
        return value;
    }

    /** Parent environment, or null pointer.
     */
    Implementation * parent_;

    /** Bindings present in the environment.
     */
    std::map<SEXP, Binding> bindings_;

    /** Assumptions for variables in the environment.
     */
    std::map<SEXP, Assumption *> assumptions_;


};


/** Parallel environments.

  It is illegal to have parallel environment with only one member.

 */
class ParallelEnvironment : public Environment::Implementation {
public:
    /** Variable lookup in parallel environments.

       The execution mode immediately changes to `maybe` and a merge of all returned values is returned.

       The bindings might have to be evaluated which may be multiple promises in many parallel environments, or even their parents. This is however not as bas as it sounds: First, the order of execution of these promises is not a problem as in practice, only one path will ever be executed so if all executions start as if they are the first, all will be fine. Furthermore, because of the `maybe` execution mode merging sideeffects, they can execute in arbitrary order at the cost of precison, but without giving up soudness.
     */
    AVALUE findVariable(SEXP name, Execution::Mode mode) override {
        assert(environments_.size() > 1);
        AVALUE result = AVALUE::bottom();
        for (Implementation * i : environments_)
            result.merge(i->findVariable(name, Execution::Mode::maybe));
        return result;
    }

    /** Closure lookup in parallel environments.

      Similarly to variable lookup, closure lookup in parallel environments is the merge of closure lookups in its child environments.
     */
    AVALUE findClosure(SEXP name, Execution::Mode mode) override {
        assert(environments_.size() > 1);
        AVALUE result = AVALUE::bottom();
        for (Implementation * i : environments_)
            result.merge(i->findClosure(name, Execution::Mode::maybe));
        return result;
    }

    /** Assignment of a variable in parallel environments.

      The value is `maybe` assigned to the parallel environments.
     */
    void assign(SEXP name, AVALUE value, Execution::Mode mode) override {
        assert(environments_.size() > 1);
        for (Implementation * i : environments_)
            i->assign(name, value, Execution::Mode::maybe);
    }

    /** Superassignment for parallel environments.

      Similarly to other parallel environment functions changes the execution mode to `maybe` and delegates to the respective environments.
     */
    void superassign(SEXP name, AVALUE value, Execution::Mode mode) override {
        assert(environments_.size() > 1);
        for (Implementation * i : environments_)
            i->superassign(name, value, Execution::Mode::maybe);
    }

    /** Returns the parent environment.

      Parent environment is a parallel environment of all parents. Technically, this may not be a parallel environment if they all share same parent, in which case the parallel environment holder is skipped and the single environment returned directly. This is to avoid the degenerate case with parallel environment having only one child.
     */
    Implementation * parent() override {
        if (parent_->environments_.size() == 1)
            return * parent_->environments_.begin();
        else
            return parent_;
    }

    /** Virtual destructor. Just checks there are no active handles.
     */
    ~ParallelEnvironment() override {
        parent_->detach();
        for (Implementation * i : environments_)
            i->detach();
    }

    // TODO missing memory management

protected:
    std::set<Implementation *> environments_;

    ParallelEnvironment * parent_;


};




} // namespace rir
