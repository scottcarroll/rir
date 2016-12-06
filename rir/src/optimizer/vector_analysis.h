//
// Created by nohajc on 6.12.16.
//

#pragma once

#include <set>
#include <algorithm>
#include <cstdint>

#include "../ir/CodeEditor.h"
#include "../code/framework.h"
#include "../code/analysis.h"



namespace rir {
    /** Abstract location:
     *  set of possible objects (addresses) the variable can point to
     */

    typedef uintptr_t AAddr;

    class ALoc {
    public:
        enum class LocType {
            bottom,
            set,
            top
        };

        ALoc() {
            locType = LocType::bottom;
        }

        bool mergeWith(ALoc const & other) {
            // bottom + bottom
            // top + top
            if (locType != LocType::set && locType == other.locType) {
                return false;
            }

            // bottom + _
            // _ + top
            if (locType == LocType::bottom || other.locType == LocType::top) {
                locType = other.locType;
                locSet = other.locSet;
                return true;
            }

            // top + _
            // _ + bottom
            if (locType == LocType::top) {
                return false;
            }

            // set + set
            if(std::includes(locSet.begin(), locSet.end(), other.locSet.begin(), other.locSet.end())) {
                return false;
            }

            locSet.insert(other.locSet.begin(), other.locSet.end());
            return true;
        }

        LocType locType;
        std::set<AAddr> locSet; // We need ordered set so that we can do efficient "is subset" operation
    };
}
