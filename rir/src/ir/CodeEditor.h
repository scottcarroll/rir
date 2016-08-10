#ifndef RIR_CODE_EDITOR
#define RIR_CODE_EDITOR

#include "BC_inc.h"

#include "utils/FunctionHandle.h"
#include "utils/CodeHandle.h"

#include <set>
#include <unordered_map>
#include <cassert>

#include <iostream>

namespace rir {

class CodeEditor {
  private:
    friend class Cursor;

    struct BytecodeList {
        BC bc;
        SEXP src = nullptr;
        BytecodeList* next = nullptr;
        BytecodeList* prev = nullptr;
        BytecodeList(BC bc) : bc(bc) {}
        BytecodeList() {}
    };

    /** Marks given label to point to a list node containing the label instruction.
     */
    void setLabel(Label index, BytecodeList* bc) {
        assert (bc->bc.bc == BC_t::label);
        if (labels_.size() < index + 1)
            labels_.resize(index + 1);
        labels_[index] = bc;
    }



    Label nextLabel = 0;

    BytecodeList front;
    BytecodeList last;
    std::vector<CodeEditor*> promises;

    SEXP ast;
    bool changed_ = false;

    std::vector<BytecodeList*> labels_;

  public:

    class Cursor {
        CodeEditor* editor_;
        BytecodeList* pos;

      public:
        Cursor(CodeEditor* editor, BytecodeList* pos)
            : editor_(editor), pos(pos) {
        }

        CodeEditor & editor() {
            return *editor_;
        }

        Label mkLabel() { return editor_->nextLabel++; }

        bool atEnd() const { return pos->next == nullptr; }

        bool firstInstruction() const { return pos->prev == nullptr; }

        Cursor & operator++() {
            assert(!atEnd());
            pos = pos->next;
            return *this;
        }

        Cursor & operator--() {
            assert(!firstInstruction());
            pos = pos->prev;
            return *this;
        }

        Cursor operator++(int) {
            Cursor result = *this;
            ++(*this);
            return result;
        }

        Cursor operator--(int) {
            Cursor result = *this;
            --(*this);
            return result;
        }

        /** The editor and range must be the same for comparison of cursors.
         */
        bool operator == (Cursor const & other) const {
            return pos == other.pos and editor_ == other.editor_;
        }

        bool operator != (Cursor const & other) const {
            return pos != other.pos or editor_ != other.editor_;
        }

        BC operator*() const { return pos->bc; }

        Cursor& operator<<(BC bc) {
            editor_->changed_ = true;

            auto insert = new BytecodeList(bc);
            // if label is inserted, update the list of labels
            if (bc.bc == BC_t::label)
                editor_->setLabel(bc.immediate.offset, insert);

            BytecodeList* prev = pos->prev;
            BytecodeList* next = pos;

            prev->next = insert;
            next->prev = insert;

            insert->prev = prev;
            insert->next = next;

            pos = next;
            return *this;
        }

        bool hasAst() { return pos->src; }

        SEXP ast() { return pos->src; }

        Cursor& operator<<(CodeEditor& other) {
            editor_->changed_ = true;

            size_t labels = editor_->nextLabel;
            size_t proms = editor_->promises.size();

            std::unordered_map<fun_idx_t, fun_idx_t> duplicate;

            fun_idx_t j = 0;
            for (auto& p : other.promises) {
                bool found = false;
                for (fun_idx_t i = 0; i < editor_->promises.size(); ++i) {
                    if (p == editor_->promises[i]) {
                        duplicate[j] = i;
                        editor_->promises.push_back(nullptr);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // We own the promise now
                    editor_->promises.push_back(p);
                    p = nullptr;
                }
                j++;
            }

            bool first = false;
            Cursor cur = other.getCursor();
            while (!cur.atEnd()) {
                *this << *cur;

                BytecodeList* insert = pos->prev;
                insert->src = cur.pos->src;
                if (first) {
                    if (!insert->src)
                        insert->src = cur.editor_->ast;
                    first = false;
                }

                // Fix prom offsets
                if (insert->bc.isCall()) {
                    fun_idx_t* args = insert->bc.immediateCallArgs();
                    num_args_t nargs = insert->bc.immediateCallNargs();
                    for (unsigned i = 0; i < nargs; ++i) {
                        if (args[i] > MAX_ARG_IDX)
                            continue;

                        if (duplicate.count(args[i]))
                            args[i] = duplicate.at(args[i]);
                        else
                            args[i] += proms;
                    }
                }
                // Fix labels
                if (insert->bc.bc == BC_t::label) {
                    insert->bc.immediate.offset += labels;
                    editor_->setLabel(insert->bc.immediate.offset, insert);
                }
                // Adjust jmp targets
                if (insert->bc.isJmp()) {
                    insert->bc.immediate.offset += labels;
                }

                ++cur;
            }
            editor_->nextLabel += other.nextLabel;

            // TODO: that stinks, I know
            delete &other;
            return *this;
        }

        void addAst(SEXP ast) {
            editor_->changed_ = true;

            assert(!pos->src);
            pos->src = ast;
        }

        void remove() {
            editor_->changed_ = true;

            assert(!atEnd());
            assert(pos != & editor_->front);

            BytecodeList* prev = pos->prev;
            BytecodeList* next = pos->next;

            prev->next = next;
            next->prev = prev;

            delete pos;
            pos = next;
        }

        bool empty() const { return editor_->empty(); }

        void print();
    };

    bool empty() const {
        return front.next == & last;
    }

    bool changed() const {
        return changed_;
    }

    Cursor begin() {
        return Cursor(this, front.next);
    }

    Cursor end() {
        return Cursor(this, & last);
    }

    /** Returns cursor to given label (the label instruction).
     */
    Cursor label(Label l) {
        return Cursor(this, labels_[l]);
    }

    Cursor getCursor() {
        return Cursor(this, front.next);
    }

    Cursor getCursorAtEnd() {
        return Cursor(this, & last);
    }

    CodeEditor(FunctionHandle function);
    CodeEditor(FunctionHandle function, fun_idx_t idx);
    CodeEditor(CodeHandle code);

    void loadCode(FunctionHandle function, CodeHandle code);

    ~CodeEditor();

    unsigned write(FunctionHandle& function);

    FunctionHandle finalize();

    void normalizeReturn();

    void print();

    size_t numPromises() const {
        return promises.size();
    }

    CodeEditor & promise(size_t index) {
        assert(index < promises.size());
        return * promises[index];
    }

    CodeEditor* detachPromise(fun_idx_t idx) {
        CodeEditor* e = promises[idx];
        promises[idx] = nullptr;
        return e;
    }

};
}

#endif
