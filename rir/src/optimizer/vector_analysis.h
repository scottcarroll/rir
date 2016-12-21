//
// Created by nohajc on 6.12.16.
//

#pragma once

#include <set>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include "ir/CodeEditor.h"
#include "code/framework.h"
#include "code/analysis.h"

namespace rir {
    /** Abstract location:
     *  set of possible objects (addresses) a variable can point to
     */

    class ALoc {
    public:
        enum class LocType {
            bottom,
            ignore,
            set,
            top
        };

        typedef unsigned long AAddr;
        typedef std::set<AAddr> LocSet;

        typedef int32_t ARef;
        typedef std::unordered_map<AAddr, ARef> MemTab;

        static const ARef ARef_NONE;
        static const ARef ARef_UNIQUE;
        static const ARef ARef_SHARED;

        ALoc(LocType type = LocType::bottom, const LocSet & set = {}) : locType(type), locSet(set) {}

        ALoc & refCntAssign(const ALoc & other, MemTab & mem_tab) {
            // Update reference count of lvalue and rvalue
            if (locType == LocType::set) {
                Rprintf("\t\t\tDECREF: ");
                print(mem_tab);
                Rprintf(" -> ");

                for (AAddr loc: locSet) {
                    mem_tab[loc]--;
                }

                print(mem_tab);
                Rprintf("\n");
            }
            // Now we have to increase the reference count of every possible rvalue
            if (other.locType == LocType::set) {
                Rprintf("\t\t\tINCREF: ");
                other.print(mem_tab);
                Rprintf(" -> ");

                for (AAddr loc: other.locSet) {
                    mem_tab[loc]++;
                }

                other.print(mem_tab);
                Rprintf("\n");
            }

            locType = other.locType;
            locSet = other.locSet;
            return *this;
        }

        const ALoc & refCntSubassign(const ALoc & other, MemTab & mem_tab) const {
            if (other.locType == LocType::set) {
                for (AAddr loc: other.locSet) {
                    mem_tab[loc]++;
                }
            }

            return *this;
        }

        bool shouldBeCopied(MemTab & mem_tab) const {
            if (locType == LocType::ignore) return false;
            if (locType == LocType::top) return true;

            // Rprintf("Vector locations:\n");
            for (AAddr loc : locSet) {
                // Rprintf("\t%d: %d\n", loc, mem_tab[loc]);
                if (mem_tab[loc] > ARef_UNIQUE) {
                    return true;
                }
            }

            return false;
        }

        static const ALoc & top() {
            static ALoc val(LocType::top);
            return val;
        }

        static const ALoc & ignore() {
            static ALoc val(LocType::ignore);
            return val;
        }

        static const ALoc & bottom() {
            static ALoc val;
            return val;
        }

        static const ALoc & Absent() {
            return bottom();
        }

        static ALoc new_obj(AAddr id, ARef rc, MemTab & mem_tab) {
            mem_tab[id] = rc;
            Rprintf("\t\t\tNEW OBJ: %u\n", id);
            return ALoc(LocType::set, {id});
        }

        bool mergeWith(const ALoc & other) {
            // bottom + bottom
            // top + top
            // ignore + ignore
            if (locType != LocType::set and locType == other.locType) {
                return false;
            }

            // bottom + _
            // _ + top
            if (locType == LocType::bottom or other.locType == LocType::top) {
                locType = other.locType;
                locSet = other.locSet;
                return true;
            }

            // top + _
            // _ + bottom
            if (locType == LocType::top or other.locType == LocType::bottom) {
                return false;
            }

            // ignore + _
            // _ + ignore
            if (locType == LocType::ignore or other.locType == LocType::ignore) {
                locType = LocType::top;
                return true;
            }

            // set + set
            if(std::includes(locSet.begin(), locSet.end(), other.locSet.begin(), other.locSet.end())) {
                return false;
            }

            locSet.insert(other.locSet.begin(), other.locSet.end());
            return true;
        }

        void print(const MemTab & mem_tab) const {
            if (locType == LocType::top) {
                Rprintf("T");
            }
            else if (locType == LocType::bottom) {
                Rprintf("B");
            }
            else if (locType == LocType::ignore) {
                Rprintf("I");
            }
            else {
                Rprintf("{");
                auto it = locSet.begin();
                if (it != locSet.end()) {
                    Rprintf("%d%c", *it, getFlag(mem_tab.at(*it)));
                    it++;

                    for(; it != locSet.end(); it++) {
                        Rprintf(", %d%c", *it, getFlag(mem_tab.at(*it)));
                    }
                }
                Rprintf("}");
            }
        }

        LocType locType;
        std::set<AAddr> locSet; // We need ordered set so that we can do efficient "is subset" operation

    private:
        static char getFlag(ARef r);
    };

    const ALoc::ARef ALoc::ARef_NONE = 0;
    const ALoc::ARef ALoc::ARef_UNIQUE = 1;
    const ALoc::ARef ALoc::ARef_SHARED = 2;

    char ALoc::getFlag(ALoc::ARef r) {
        switch (r) {
            case ARef_NONE:
                return 'N';
            case ARef_UNIQUE:
                return 'U';
            default:
                return 'S';
        }
    }


    template <typename AVALUE>
    class RefCountAbstractState : public AbstractState<AVALUE> {
    public:
        typedef typename AVALUE::ARef ARef;
        typedef std::unordered_map<typename AVALUE::AAddr, ARef> MemTab;

        RefCountAbstractState() = default;
        RefCountAbstractState(RefCountAbstractState const &) = default;

        RefCountAbstractState * clone() const override {
            return new RefCountAbstractState(*this);
        }

        bool mergeWith(State const * other) override {
            assert(dynamic_cast<RefCountAbstractState const*>(other) != nullptr);
            return mergeWith(*dynamic_cast<RefCountAbstractState const *>(other));
        }

        bool mergeWith(RefCountAbstractState const & other) {
            bool result = AbstractState<AVALUE>::mergeWith(other);
            bool changed = false;

            // Merge reference counts conservatively (take the maximum of each location's ref. count)

            for (auto & kv : memTab_) {
                auto it = other.memTab_.find(kv.first);
                if (it != other.memTab_.end()) {
                    // There is the same allocation site in the other
                    // memTab with potentially greater refcount.
                    ARef rc = kv.second;
                    ARef other_rc = it->second;
                    if (rc < other_rc) {
                        kv.second = other_rc;
                        changed = true;
                    }
                }
            }

            for (auto & kv : other.memTab_) {
                auto it = memTab_.find(kv.first);
                if (it == memTab_.end()) {
                    // There is a new allocation site in the other memTab
                    // not included in this one. We have to add it.
                    memTab_[kv.first] = kv.second;
                    changed = true;
                }
            }

//            std::transform(memTab_.begin(), memTab_.end(),
//                           other.memTab_.begin(),
//                           memTab_.begin(), [&changed](ARef a, ARef b) {
//                Rprintf("Merging %u, %u\n", a, b);
//                if (a >= b) {
//                    return a;
//                }
//                else {
//                    // We have taken at least one value from the other vector
//                    // => the original (our) vector is changed
//                    changed = true;
//                    return b;
//                }
//            });

            return result or changed;
        }

        MemTab & memTab() {
            return memTab_;
        }

        MemTab const& memTab() const {
            return memTab_;
        }

    private:
        MemTab memTab_;
    };


    class VectorAnalysis : public ForwardAnalysisFinal<RefCountAbstractState<ALoc>>, public InstructionDispatcher::Receiver {
    public:
        VectorAnalysis() : dispatcher_(*this) {}

//        AbstractState<ALoc> *initialState() override {
//            AbstractState<ALoc> * result = new AbstractState<ALoc>();
//            for (auto& x : code_->arguments()) {
//                (*result)[x.first] = ALoc(ALoc::LocType::top);
//            }
//
//            return result;
//        }

        void push_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            bc.print();
            SEXP val = bc.immediateConst();
            auto & curr = current();

            //Rprintf("TYPEOF(val) = %d\n", TYPEOF(val));

            switch (TYPEOF(val)) {
                case VECSXP:
                case INTSXP:
                case REALSXP:
                case CPLXSXP:
                case LGLSXP:
                case STRSXP:
                case RAWSXP:
                    curr.push(new_obj(ins, ALoc::ARef_UNIQUE));
                    break;
                default:
                    curr.push(ALoc::ignore());
            }
        }

        void dup_(CodeEditor::Iterator ins) override {
            (*ins).print();
            auto & curr = current();
            curr.push(curr.top());
        }

        void uniq_(CodeEditor::Iterator ins) override {
            (*ins).print();
            auto & curr = current();
            if (curr.top().shouldBeCopied(curr.memTab())) {
                curr.top() = new_obj(ins);
            }
        }

        void swap_(CodeEditor::Iterator ins) override {
            (*ins).print();
            auto & curr = current();
            ALoc a = curr.pop();
            ALoc b = curr.pop();
            curr.push(a);
            curr.push(b);
        }

        void pick_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            bc.print();
            int n = bc.immediate.i;
            auto & curr = current();
            ALoc v = curr[n];

            for (int i = n; i > 0; i--) {
                curr[i] = curr[i - 1];
            }
            curr.top() = v;
        }

        void stvar_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            bc.print();
            SEXP vName = bc.immediateConst();
            varNames.insert(vName);
            auto & curr = current();
            refCntAssign(curr[vName], curr.pop());

            const char * str = CHAR(PRINTNAME(vName));
            Rprintf("\t\t\t%s = ", str);
            curr[vName].print(curr.memTab());
            Rprintf("\n");
        }

        void ldvar_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            bc.print();
            SEXP vName = bc.immediateConst();
            varNames.insert(vName);
            auto & curr = current();
            curr.push(curr[vName]);

            const char * str = CHAR(PRINTNAME(vName));
            Rprintf("\t\t\t%s = ", str);
            curr[vName].print(curr.memTab());
            Rprintf("\n");
        }

        void subassign_helper(CodeEditor::Iterator ins, const ALoc & vec, const ALoc & val) {
            auto & curr = current();

//            Rprintf("SUBASSIGN:\n");
//            Rprintf("\tvec = "); vec.print(mem_tab);
//            Rprintf("\n\tval = "); val.print(mem_tab);
//            Rprintf("\n");

            refCntSubassign(vec, val);
            curr.pop(3);

            if (vec.shouldBeCopied(curr.memTab())) {
                curr.push(new_obj(ins));
            }
            else {
                curr.push(vec);
            }
        }

        void subassign_(CodeEditor::Iterator ins) override {
            (*ins).print();
            auto & curr = current();
            ALoc val = curr.stack()[2];

            // We don't care about the index since
            // we don't model vector element pointers.
            ALoc vec = curr.stack()[0];
            subassign_helper(ins, vec, val);
        }

        void dispatch_stack_(CodeEditor::Iterator ins) override {
            (*ins).print();
            auto & curr = current();
            CallSite cs = ins.callSite();
            std::string fnName = CHAR(PRINTNAME(cs.selector()));

            if (fnName == "[<-") { // Assuming this never gets redefined
                ALoc val = curr.stack()[0];
                ALoc vec = curr.stack()[2];
                subassign_helper(ins, vec, val);
            }
            else {
                curr.pop(3);
                curr.push(ALoc::top());
            }
        }

        void generic_binop(CodeEditor::Iterator ins) {
            (*ins).print();

            auto & curr = current();
            curr.pop(2);
            curr.push(new_obj(ins));
        }

        void idiv_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void div_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void mod_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void add_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void pow_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void mul_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void lt_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

        void sub_(CodeEditor::Iterator ins) override {
            generic_binop(ins);
        }

//        void any(CodeEditor::Iterator ins) override {
//            Rprintf("Called any: ");
//            (*ins).print();
//        }

        void print() override {
            auto & final = finalState();
            Rprintf("Vector analysis:\n");

            for (auto & var : varNames) {
                const char * str = CHAR(PRINTNAME(var));
                Rprintf("%s = ", str);
                finalState()[var].print(final.memTab());
                Rprintf("\n");
            }
        }

    protected:
        ALoc & refCntAssign(ALoc & a, const ALoc & b) {
            return a.refCntAssign(b, current().memTab());
        }

        const ALoc & refCntSubassign(const ALoc & a, const ALoc & b) {
            return a.refCntSubassign(b, current().memTab());
        }

        ALoc new_obj(CodeEditor::Iterator ins, ALoc::ARef rc = ALoc::ARef_NONE) {
            unsigned id = ins.asCursor(*code_).srcIdx();
            return ALoc::new_obj(id, rc, current().memTab());
        }

        Dispatcher & dispatcher() override {
            return dispatcher_;
        }

    private:
        InstructionDispatcher dispatcher_;

        std::set<SEXP> varNames;
        //std::vector<ALoc::ARef> mem_tab;
    };
}
