//
// Created by nohajc on 6.12.16.
//

#pragma once

#include <set>
#include <vector>
#include <algorithm>
#include <cstdint>

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
            set,
            top
        };

        typedef uintptr_t AAddr;
        typedef std::set<AAddr> LocSet;

        typedef int32_t ARef;
        static const ARef ARef_NONE;
        static const ARef ARef_UNIQUE;
        static const ARef ARef_SHARED;

        ALoc(LocType type = LocType::bottom, const LocSet & set = {}) : locType(type), locSet(set) {}

        ALoc & refCntAssign(const ALoc & other, std::vector<ARef> & mem_tab) {
            // Update reference count of lvalue and rvalue
            if (locType == LocType::set and locSet.size() == 1) {
                // The only case when we can unambiguously decrease reference count of lvalue
                mem_tab[*locSet.begin()]--;
            }
            // Now we have to increase the reference count of every possible rvalue
            if (other.locType == LocType::set) {
                for (AAddr loc: other.locSet) {
                    mem_tab[loc]++;
                }
            }

            locType = other.locType;
            locSet = other.locSet;
            return *this;
        }

        const ALoc & refCntSubassign(const ALoc & other, std::vector<ARef> & mem_tab) const {
            if (other.locType == LocType::set) {
                for (AAddr loc: other.locSet) {
                    mem_tab[loc]++;
                }
            }

            return *this;
        }

        bool shouldBeCopied(std::vector<ARef> & mem_tab) const {
            if (locType == LocType::bottom) return false;
            if (locType == LocType::top) return true;

            Rprintf("Vector locations:\n");
            for (AAddr loc : locSet) {
                Rprintf("\t%d: %d\n", loc, mem_tab[loc]);
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

        static const ALoc & bottom() {
            static ALoc val;
            return val;
        }

        static const ALoc & Absent() {
            return bottom();
        }

        static ALoc new_obj(ARef rc, std::vector<ARef> & mem_tab) {
            mem_tab.push_back(rc);
            AAddr a = mem_tab.size() - 1;
            return ALoc(LocType::set, {a});
        }

        bool mergeWith(const ALoc & other) {
            // bottom + bottom
            // top + top
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

            // set + set
            if(std::includes(locSet.begin(), locSet.end(), other.locSet.begin(), other.locSet.end())) {
                return false;
            }

            locSet.insert(other.locSet.begin(), other.locSet.end());
            return true;
        }

        void print(const std::vector<ARef> & mem_tab) const {
            if (locType == LocType::top) {
                Rprintf("T");
            }
            else if (locType == LocType::bottom) {
                Rprintf("B");
            }
            else {
                Rprintf("{");
                auto it = locSet.begin();
                if (it != locSet.end()) {
                    Rprintf("%d%c", *it, mem_tab[*it] > 1 ? 'S' : 'U');
                    it++;

                    for(; it != locSet.end(); it++) {
                        Rprintf(", %d%c", *it, mem_tab[*it] > 1 ? 'S' : 'U');
                    }
                }
                Rprintf("}");
            }
        }

        LocType locType;
        std::set<AAddr> locSet; // We need ordered set so that we can do efficient "is subset" operation
    };

    const ALoc::ARef ALoc::ARef_NONE = 0;
    const ALoc::ARef ALoc::ARef_UNIQUE = 1;
    const ALoc::ARef ALoc::ARef_SHARED = 2;

    using AStateALoc = AbstractState<ALoc>;

    class VectorAnalysis : public ForwardAnalysisFinal<AStateALoc>, public InstructionDispatcher::Receiver {
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
                    curr.push(new_obj(ALoc::ARef_UNIQUE));
                    break;
                default:
                    curr.push(ALoc::bottom());
            }
        }

        void dup_(CodeEditor::Iterator ins) override {
            auto & curr = current();
            curr.push(curr.top());
        }

        void uniq_(CodeEditor::Iterator ins) override {
            auto & curr = current();
            if (curr.top().shouldBeCopied(mem_tab)) {
                curr.top() = new_obj();
            }
        }

        void swap_(CodeEditor::Iterator ins) override {
            auto & curr = current();
            ALoc a = curr.pop();
            ALoc b = curr.pop();
            curr.push(a);
            curr.push(b);
        }

        void pick_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            int n = bc.immediate.i;
            auto & curr = current();
            ALoc v = curr[n];

            for (int i = 0; i < n - 1; i++) {
                curr[i + 1] = curr[i];
            }
            curr.top() = v;
        }

        void stvar_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            SEXP vName = bc.immediateConst();
            varNames.insert(vName);
            auto & curr = current();
            refCntAssign(curr[vName], curr.pop());
        }

        void ldvar_(CodeEditor::Iterator ins) override {
            BC bc = *ins;
            SEXP vName = bc.immediateConst();
            varNames.insert(vName);
            auto & curr = current();
            curr.push(curr[vName]);
        }

        void subassign_helper(const ALoc & vec, const ALoc & val) {
            auto & curr = current();

            refCntSubassign(vec, val);
            curr.pop(3);

            if (vec.shouldBeCopied(mem_tab)) {
                curr.push(new_obj());
            }
            else {
                curr.push(vec);
            }
        }

        void subassign_(CodeEditor::Iterator ins) override {
            Rprintf("subassign_\n");
            auto & curr = current();
            ALoc val = curr.stack()[2];
            // We don't care about the index since
            // we don't model vector element pointers.
            ALoc vec = curr.stack()[0];
            subassign_helper(vec, val);
        }

        void dispatch_stack_(CodeEditor::Iterator ins) override {
            Rprintf("dispatch_stack_\n");
            auto & curr = current();
            CallSite cs = ins.callSite();
            std::string fnName = CHAR(PRINTNAME(cs.selector()));

            if (fnName == "[<-") { // Assuming this never gets redefined
                ALoc val = curr.stack()[0];
                ALoc vec = curr.stack()[2];
                subassign_helper(vec, val);
            }
            else {
                curr.pop(3);
                curr.push(ALoc::top());
            }
        }

        void generic_binop(CodeEditor::Iterator ins) {
            auto & curr = current();
            curr.pop(2);
            curr.push(new_obj());
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
            Rprintf("Vector analysis:\n");

            for (auto & var : varNames) {
                const char * str = CHAR(PRINTNAME(var));
                Rprintf("%s: ", str);
                finalState()[var].print(mem_tab);
                Rprintf("\n");
            }
        }

    protected:
        ALoc & refCntAssign(ALoc & a, const ALoc & b) {
            return a.refCntAssign(b, mem_tab);
        }

        const ALoc & refCntSubassign(const ALoc & a, const ALoc & b) {
            return a.refCntSubassign(b, mem_tab);
        }

        ALoc new_obj(ALoc::ARef rc = ALoc::ARef_NONE) {
            return ALoc::new_obj(rc, mem_tab);
        }

        Dispatcher & dispatcher() override {
            return dispatcher_;
        }

    private:
        InstructionDispatcher dispatcher_;

        std::set<SEXP> varNames;
        std::vector<ALoc::ARef> mem_tab;
    };
}
