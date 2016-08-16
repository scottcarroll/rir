#ifndef RIR_ANALYSIS_H
#define RIR_ANALYSIS_H

#include "ForwardDriver.h"
#include "state.h"

namespace rir {

typedef State ASTATE;


/** Analysis base class.

  An analysis should be able to do two things:

  - analyze given code
  - get the results of the analysis during the optimization phase

  Analysis is templated by the state


 */
template <typename ASTATE>
class Analysis {
public:


    // I'd like the const here
    void analyze(CodeEditor & code) {
        editor_ = & code;
        doAnalyze(code);
    }

    /** Invalidates the analysis results. Descendants should override this and add any cleanup if necessary.
      */
    virtual void invalidate() {
        editor_ = nullptr;
    }

    /** An analysis is valid when its editor is not null.
     */
    bool isValid() const {
        return editor_ != nullptr;
    }

    ASTATE const & operator [] (CodeEditor::Cursor const & ins) const {
        assert(& ins.editor() == editor_ and "you can only use cursors from the same editor");
        return stateFor(ins);
    }

    /** This would allow modifying the state of the analysis when optimizing, but is disabled for now.
     */
    ASTATE & operator [] (CodeEditor::Cursor const & ins) = delete;


    /** Debug print of the cached analysis state.
     */
    void print() {
        if (not isValid()) {
            Rprintf("NOT VALID");
            return;
        }
    }

protected:

    /** Provide implementation for the code that actually does the analysis.
     */
    virtual void doAnalyze(CodeEditor & code) = 0;

    /** Given a cursor, returns the state at that cursor.
     */
    virtual ASTATE const & stateFor(CodeEditor::Cursor const & ins) const = 0;


    CodeEditor const * editor_ = nullptr;
};

template <typename ASTATE>
class ForwardAnalysis : public Analysis<ASTATE>, public ForwardDriver {
public:
    void invalidate() override {
        Analysis<ASTATE>::invalidate();
        cleanup();
        delete currentState_;
        currentState_ = nullptr;
    }



protected:
  using Analysis<ASTATE>::editor_;

  ASTATE& current() {
      // TODO this can look better if ForwardDriver is also templated
      return *reinterpret_cast<ASTATE*>(currentState_);
  }

  ASTATE const& current() const {
      // TODO this can look better if ForwardDriver is also templated
      return *reinterpret_cast<ASTATE*>(currentState_);
  }

  ForwardAnalysis() : ForwardDriver(), dispatcher_(nullptr) {}

    /** The analysis just runs the forward driver on the given code with the dispatcher.
     */
    void doAnalyze(CodeEditor & code) override {
        assert(dispatcher_ != nullptr);
        doRun(code, *dispatcher_);
        // prepare the cache for retrieval
        initializeCache();
    }

    ASTATE const& stateFor(CodeEditor::Cursor const& ins) const override {
        // if the cached result is not the one we want, seek to it
        if (ins != cached_)
            seek(ins);
        return current();
    }

    void advance() const {
        assert(dispatcher_ != nullptr);
        dispatcher_->dispatch(cached_);
        ++cached_;
        // if the cached instruction is label, dispose of the state and create a copy of the fixpoint
        if (cached_->bc == BC_t::label) {
            delete currentState_;
            currentState_ = storedState(cached_)->clone();
        }
    }

    void initializeCache() const {
        delete currentState_;
        currentState_ = initialState_->clone();
        cached_ = editor_->begin();
    }

    void seek(CodeEditor::Cursor const & ins) const {
        CodeEditor::Cursor end = ins.editor().end();
        while (cached_ != end) {
            if (cached_ == ins)
                return;
            // advance the state using dispatcher
            advance();
        }
        // if we haven't found it, let's start over
        initializeCache();
        while (cached_!= end) {
            if (cached_ == ins)
                return;
            advance();
        }
        assert(false and "not reachable");
    }

    /** The dispatcher used in the analysis, which will then be reused in the retrieval phase. */
    Dispatcher* dispatcher_;

    /** Cursor used for the state retrieval. */
    mutable CodeEditor::Cursor cached_;

};








}
#endif
