#pragma once

#include <cassert>


#include "avalue.h"
#include "assumption.h"
#include "state.h"
#include "execution.h"

#include "../R/r.h"

/** TODO & IDEAS

  The code assumes some stop environment instead of no environment. We might need two types - one that returns bottom for everything, which signifies that it might not be found and one that returns top for each variable which says that the variable can be anything.

  Therefore parent_ null checks might be not necessary.


  # Corrupted environment

  Corrupted environment is an environment that assumes for each symbol name that it has a maybe binding of this symbol to Top, which has an unevaluated worst case promise. There can be a flag on the environment and we can use this to account for worst case scenarios of unknown code being executed.

  # Non-abstract value assumptions

  Since the analysis will execute other code if known, or not known, it must be able to provide assumptions about this regardless of its abstract domain.

  # Templates

  Any class that uses AVALUE in this version of implementation will be templated by AVALUE.

  # Caching of evaluated promise values

  When we go to optimization phase, the former implementation was that the analysis only stores the state at control flow joints and recomputes the seqential information as there is linear penalty only. This is not the case when promises may be evaluated - so their results can be cached.

  Also perhaps, the analysis can store environment diffs and only update the optimization state by them as it runs...

  TODO The question also is what & how to store at the merge points - this is easy for local information, but now when environment changes non-locally, everything that points to it should be reanalyzed if it is on the execution stack, which the analysis now has to model (but when it is on the execution stack, then it would most likely be reanalyzed anyways when execution resumes.

  !!!! What if it is *not* on the execution stack - I think then we should not worry about it unless we execute it at some point, in which case it would be rerun, if we do not reexecute (and the ANALYSIS MUST MAKE SURE THAT IT REEXECUTES ANYTHING THAT CAN BE REEXECUTED) then we won't even optimize upon it



 */

namespace rir {

class Analysis;

/** Environment public API.

  Will also function as smart pointer to the underlying environment implementation.
 */
class Environment : public State {
public:
    /** Returns true if the environment points to an actual implementation.
     */
    bool good() const {
        return impl_ != nullptr;
    }

    /** Finds environment that has binding for given variable name.

      Calling this has *no* sideeffects on he analysis state. Returns environment, or parallel environment in case of `maybe` bindings that may hold visible binding for the variable.

      TODO this should return something else than the environment, like some generalized binding that one can query whether the binding is always justified, or never, or so and this would then have links to the environments.
     */
    Environment findBinding(SEXP name);

    /** Normal variable lookup.
     */
    AVALUE findVariable(SEXP name);

    /** Implements R's closure lookup.
     */
    AVALUE findClosure(SEXP name);

    /** Assignment into the environment.
     */
    void assign(SEXP name, AVALUE value);

    /** R's super assignment.

      Obtains parent of the environment and performs a super assignment in it.
     */
    void superassign(SEXP name, AVALUE value);

    /** Returns the parent environment.
     */
    Environment parent() {
        assert(good());
        return Environment(impl_->parent());
    }

    class Implementation {
    public:
        virtual AVALUE findVariable(SEXP name, Execution::Mode mode) = 0;
        virtual AVALUE findClosure(SEXP name, Execution::Mode mode) = 0;
        virtual void assign(SEXP name, AVALUE value, Execution::Mode mode) = 0;
        virtual void superassign(SEXP name, AVALUE value, Execution::Mode mode) = 0;

        /** Returns the parent environment.
         */
        virtual Implementation * parent() = 0;

        /** Virtual destructor. Just checks there are no active handles.
         */
        virtual ~Implementation() {
            assert(handles_ == 0 and "Deleting environment with existing handles");
        }

    protected:
        friend class Environment;
        friend class HashMapEnvironment;
        friend class ParallelEnvironment;



        Implementation * attach() {
            ++handles_;
            return this;
        }

        Implementation * detach() {
            if (--handles_ == 0)
                delete this;
            return nullptr;
        }

        /** Each environment must have an access to its analysis so that it knows how to execute promises.
         */
        Analysis & analysis_;

    private:
        unsigned handles_;

    };

    // smart pointer behavior, kind of

    explicit Environment(Implementation * impl) {
        if (impl != nullptr)
            impl_ = impl->attach();
        else
            impl_ = nullptr;
    }

    Environment(Environment const & e) {
        if (e.impl_ != nullptr)
            impl_ = e.impl_->attach();
        else
            impl_ = nullptr;
    }

    Environment & operator = (Environment const & e) {
        if (impl_ != nullptr)
            impl_ = impl_->detach();
        if (e.impl_ != nullptr)
            impl_ = e.impl_->attach();
        return *this;
    }

private:
    friend class Implementation;
    Implementation * impl_;
};



} // namespace rir

#ifdef HAHA



/** The abstract environment.

  Two kinds of environments exist:

  - non-virtual environments (i.e. environments attached to closures, or environments whose existence at runtime is guaranteed)
  - virtual environments (abstractions over unknown environment chains, or collections of environments when the origins of a value cannot be determined)

  The above types can be further diferentiated by different implementations (the `Environment` class is a smart pointer that also defines the public API), so that for instance parallel or sequential virtual envronments can be created, or multiple ways of local value storages can be obtained.


 */
template<typename AVALUE>
class Environment {
public:

    /** Reurns true if the environment exists.
     */
    bool good() const {
        return impl_ != nullptr;
    }

    /** Returns true if the environment is virtual, i.e. if it possibly stands for more than one real environments present at runtime (either sequetial or parallel collection of multiple environments).
     */
    bool isVirtual() const {
        return impl_->isVirtual();
    }

    /** Returns the parent environment. If parent environment does not exist, calling good() on the result will return false.
     */
    Environment parent() const {
        return impl_->parent();
    }

    // The high-level API for the

    AVALUE findVariable(SEXP name) {

    }

    AVALUE findVariable(SEXP name, Environment & env) {

    }

    void assign(SEXP name, AVALUE value, bool maybe = false) {

    }

    void superAssign(SEXP name, AVALUE value, bool maybe = false) {

    }

    /** Default constructor.
     */
    Environment():
        impl_(nullptr) {
    }



    /** As a smart pointer, destructor detaches from the implementation.
     */
    ~Environment() {
        detach();
    }

    /** Environment record.

      Because the analysis should natively support promises, we should as well support delayed assignments and all other kinds things found in R, which is why this is so complicated: Environments store records in themselves, the record contains the abstract value recorded and information about possible promises that must be evaluated to obtain proper value. The promises are encoded as a hashmap from pointers to the code to PromiseEvaluation type values. These determine for each promise, whether it has been evaluated, or not, or maybe evaluated.

      Merging the records is rather straightforward. First the values of two records are merged. Then their promise information is merged according to the following rules:

      - if certain promise is present only in the other record, it is copied as is to the result
      - if promise is present in both, then it will be evaluated in result if evaluated in both, not evaluated in result if not evaluated in both and maybe evaluated in result in all other cases.

      Unless there is no promise information in the record, in which case the value of the record is the value it stores, getting the value of the record is non-trivial:

      - if the record contains only one promise, and that is definitely not evaluated, its code is evaluated, value stored, and evaluation changed to `yes`
      - if the record contains more unevaluated (or `maybe`) promises, or if it contains single `maybe` promise, the promises are evaluated in *maybe* mode, their results are merged with the existing value and their evaluation is updated to `yes`

      # Maybe evaluation mode

      Maybe execution mode indicates that the code in question *may* be executed, but its execution is not certain. Such mode propagates through calls (if the analysis concerns itself with the calls) and has the effect of all set operations on the environment to be `maybe` sets.

      Each record also contains in itself the information whether it is present, or maybe present in the environment.

     */
    struct Record {
        /** State of promises associated with the record.
         */
        enum class PromiseEvaluation {
            no,
            maybe,
            yes
        };

        /** Value of the record.
         */
        AVALUE value;

        /** If true, the value is only maybe present in the environment.
         */
        bool maybe;

        /** Promises associated with the record and their evaluation state.
         */
        std::map<void*, PromiseEvaluation> promises;

        /** Merges two records together and returns true if the record has changed.
         */
        bool merge(Record const & other) {
            bool result = value.merge(other.value);
            for (auto const & i : other.promises) {
                auto own = promises.find(i.first);
                if (own == promises.end()) {
                    promises[i.first] = i.second;
                    result = true;
                } else {
                    result = merge(own->second, i.second) or result;
                }
            }
            // The maybe status of two records is merged so that if either of them is not maybe, the result is not maybe.
            if (maybe == true and other.maybe == false) {
                maybe = false;
                result = true;
            }
            return result;
        }

    private:
        /**    a\b  n m y
                n   n m m
                m   m m m
                y   m m y
         */
        static bool merge(PromiseEvaluation & a, PromiseEvaluation b) {
            if (a == b)
                return false;
            if (a != PromiseEvaluation::maybe) {
                a = PromiseEvaluation::maybe;
                return true;
            } else {
                return false;
            }
        }
    };

    /** The actual implementation of the environment.

     */
    class Implementation {
    public:
        /** Returns true if the environment is virtual, i.e. does not actually exist at runtime.
         */
        virtual bool isVirtual() const {
            return false;
        }

        /** Returns the parent environment.
         */
        virtual Environment parent() = 0;

        /** Finds variable with given name, executes its promises, if it has to do so and returns the value.
         */
        virtual AVALUE findVariable(SEXP name) = 0;



        /** Virtual destructor for proper behavior.
         */
        virtual ~Implementation() {
        }

    protected:
        friend class Environment;

        /** Returns the value of given record, possibly executing its promises and updating the record.
         */
        AVALUE getRecordValue(Record & rec) {
            assert(false and "Not implemented");
            return AVALUE();
        }


        /** Reference counting.
          */
        size_t handles_;
    };

protected:

    /** Constructor that attaches to the given implementation.
     */
    Environment(Implementation * implementation) {
        attach(implementation);
    }


private:

    template<typename T> // let's be friends generic
    friend class HashMapEnvironment;

    template<typename T> // let's be friends generic
    friend class ParallelEnvironment;


    /** Attaches the environment to an implementation.
     */
    void attach(Implementation * implementation) {
        assert(impl_ == nullptr and "Must detach first");
        if (implementation != nullptr) {
            ++implementation->handles_;
            impl_ = implementation;
        }
    }

    /** Detaches from the implementation.
     */
    void detach() {
        if (impl_ != nullptr) {
            if (--impl_->handles_ == 0)
                delete impl_;
            impl_ = nullptr;
        }
    }

    /** Actual implementation of the environment.
     */
    Implementation * impl_;
};


/** Generic environment that stores all its variables in a hashmap.
 */
template<typename AVALUE>
class HashMapEnvironment : public Environment<AVALUE>::Implementation {
    typedef Environment<AVALUE> Env;
public:

    /** Hashmaps environments can be set as virtual, or not. */
    bool isVirtual() const override {
        return virtual_;
    }

    /** Returns the parent.
     */
    Env parent() override {
        return parent_;
    }

    /** Finds given variable in the environment and returns its abstract value.

      If the variable is not found in the current environment, its parent environment is searched. If the variable is only maybe present in the environment, the join of the maybe stored value and the result of the variable found in the parent recursively is returned.

      The variable might be a promise in which case the promise (or promises) will be evaluated.
     */
    AVALUE findVariable(SEXP name) override {
        typename Env::Record & rec = variables_.find(name);
        // if we do not have it, return what parent says
        if (rec == variables_.end())
            return parent_.impl_->findVariable(name);
        // we have it, get the value
        AVALUE result = getRecordValue(rec);
        // if we might not have the variable, then we must merge the variable with any value that our parent might have
        if (rec.maybe)
            result.merge(parent_.impl_->findVariable(name));
        return result;
    }


protected:

    bool virtual_;
    Env parent_;

    std::map<SEXP, typename Env::Record> variables_;

};

/** Environment that holds a collection of multiple environments and represents all of them at once.

  Currently, the parallel environments do not collapse if it contains a parallel environment as a child.
 */
template<typename AVALUE>
class ParallelEnvironment : public Environment<AVALUE>::Implementation {
    typedef Environment<AVALUE> Env;
public:

    /** If parallel environment contains only one environment, then its virtuality is determined by that environment.

      Otherwise it is always virtual.
     */
    bool isVirtual() const {
        if (envs_.size() == 1)
            return envs_[0]->isVirtual();
        return true;
        // TODO what to do if empty - an empty parallel environment should act as no environment at all
    }

    /** Parent of a parallel environment is the parallel environment of all its environments' parents.

      TODO this may get tricky when some have non-existent parent (i.e. how to represent an existing non-existent parent), but maybe this is not really needed as the parent() methods are only used for manual crawls.
     */
    Environment<AVALUE> parent() override {
        if (envs_.empty())
            return Env();
        ParallelEnvironment * result = new ParallelEnvironment(envs_);
        return Env(result);
    }






    /** Adds given environment to the holder.
     */
    void add(Env e) {
        assert(e.impl_ != nullptr and "Adding non-existing environment");
        envs_.insert(e.impl_);
    }

    /** Parallel environment's destructor detaches from the environments it stores and deletes them if necessary.
     */
    ~ParallelEnvironment() {
        for (typename Env::Implementation * i : envs_) {
            if (--(i->handles_) == 0)
                delete i;
        }
    }


protected:

    ParallelEnvironment(std::set<typename Env::Implementation *> const & envs):
        envs_(envs) {
    }

    std::set<typename Env::Implementation *> envs_;
};





























/*

  Update mode == all varaibles are written as joins, or as maybe if not present before



*/











} // namespace rir



/** Environment, State and Analusis API drafts

  NOTE Currently this does not compile and will require some updates to work properly. The comments are present to discuss not only the API, but also the implementation restrictions the API poses.

 */

namespace oldstuff {

/** Abstract values.

  In practice abstract values will be always template arguments so that there will be no need to typecast everything when inside the analysis. This class is here just to show what each abstract value must support.

  Abstract values should be created in such way that they can be efficiently passed by value. This greatly simplifies the actual environment handling and memory management as we do not have to fight with references or pointers (see below with environments).
 */

class AbstractValue {
public:
    /** Abstract values must be copy constructible.
     */
    AbstractValue(AbstractValue const &) = default;

    /** Merges the other abstract value into current one. Returns true if the abstract value changed, false otherwise.

     */
    bool merge(AbstractValue const & other);

    /** Returns the supremum of itself and the other value.
     */
    AbstractValue upper(AbstractValue const & other) const {
        // upper is implemented using merge and not vice-versa for performance reasons
        AbstractValue x(*this);
        x.merge(other);
        return x;
    }

    /** Returns the infimum of itself and the other value.

      TODO this is to be used by assumptions, when the value the optimization can use
     */
    AbstractValue lower(AbstractValue const & other) const;


    /** Constructor for top values.

      NOTE We do not need constructor for bottom values as bottom is internally represented as not having any value stored, but we may still require a static bottom() method for the sake of completeness.
     */
    static AbstractValue top();
};

/** Base class for any abstract state.

  Abstract state is something that an analysis drags with itself and uses it to determine whether a fixpoint has been reached, or not. Abstract states usse virtual methods, they can be made to use only templates easily, but this is perhaps easier to understand without any severe usability drawbacks.
 */
class AbstractState {
public:
    /** Abstract state can clone itself (provide deep copy).
     */
    virtual AbstractState * clone() const = 0;

    /** Abstract state can merge with other abstract state, updating itself and returning true if it changed, false otherwise.
     */
    virtual bool merge(AbstractState const * other) = 0;
};


/** Assumptions base class.

 */
template<typename AVALUE>
class Assumption {
    AVALUE value();

};

/** Base class for all environments.

  Provides the generic API for environment access. Subclasses of Environment will the provide the actual implementation of the storage.

  Internally, the environment will become a smart pointer to an actual environment implementation. This is important to facilitate ugly operations such as returning a value and and environment in comes from when the values are maybe's.

  Superficially, an environment is a map of names to values. Each element in the map can be:

  - absent
  - maybe present
  - definitely present

  As smartpointer is super cheap to construct.
 */
template<typename AVALUE>
class Environment : public AbstractState {
public:
    /** Return true if the environment exists.
     */
    bool exists();

    /** Returns true if the environment is virtual, i.e. if it does not have a counterpart at runtime.
     */
    bool isVirtual();

    /** Returns the parent environment.

      If parent environment does not exist, the returned environment's exist() call will return false.
     */
    Environment & parent();

    /** Sets given variable to the value, the last argument is the maybe tag. If true the value is only maybe present.

      NOTE If the variable is already defined in the environment as definitely present, it should be an error, to maybe assign it.

     */
    void set(SEXP name, AVALUE const & value, bool maybe = false);

    /** Returns the value stored (bottom if not stored) and true if the value is only maybe present.
     */
    std::pair<AVALUE, bool> get(SEXP name);


    /** Performs the semantics of R's variable lookup.

      This gets funny in the presence of maybe variables and multiple defining environments.
     */
    AVALUE find(SEXP name);

    /** Performs part of R's closure lookup semantics.

      Returns value of the variable and the environment in which it was defined. This gets even funnier if defined in multiple environments, or maybes, or so.

     */
    std::Pair<AVALUE, Environment> findFirst(SEXP name);

    /** R's assignment semantics.
     */
    void assign(SEXP name, AVALUE const & value);

    /** R's super assignment semantics. */
    void superAssign(SEXP name, AVALUE const & value);


};

template<typename AVALUE>
class EnvironmentImplementation {



};














} // namespace rir


#endif
