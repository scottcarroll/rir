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
        static const ARef ARef_UNIQE;
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

        static ALoc new_obj(std::vector<ARef> & mem_tab) {
            mem_tab.push_back(ARef_NONE);
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
                        Rprintf(", %d", *it);
                    }
                }
                Rprintf("}");
            }
        }

        LocType locType;
        std::set<AAddr> locSet; // We need ordered set so that we can do efficient "is subset" operation
    };

    const ALoc::ARef ALoc::ARef_NONE = 0;
    const ALoc::ARef ALoc::ARef_UNIQE = 1;
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

        void push_(CodeEditor::Iterator) override {
            current().push(new_obj());
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
        ALoc new_obj() {
            return ALoc::new_obj(mem_tab);
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
