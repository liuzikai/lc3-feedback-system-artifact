//
// Created by liuzikai on 6/15/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_STATEALLOCATOR_HPP
#define KLC3_STATEALLOCATOR_HPP

#include "State.h"

namespace klc3 {

/**
 * This class manage the life cycle of States.
 * When releasing a state, it's user's responsibility to make sure the pointer to be released is not used elsewhere.
 */
class StateAllocator {
public:

    State *createInitState(ExprBuilder *builder, ref<MemValue> *baseMemArray, uint16_t initPC, IssuePackage *issuePackage) {
        auto *ret = new State(builder, baseMemArray, initPC, issuePackage);
        ret->uid = totalAllocatedStateCount;
        totalAllocatedStateCount++;
        aliveStates.insert(ret);
        return ret;
    }

    State *fork(State *state) {
        State *ret = state->fork();
        ret->uid = totalAllocatedStateCount;
        totalAllocatedStateCount++;
        aliveStates.insert(ret);
        return ret;
    }

    void releaseState(State *state) {
        auto it = aliveStates.find(state);
        assert(it != aliveStates.end());
        aliveStates.erase(it);
        delete state;
    }

    ~StateAllocator() {
        for (State *it: aliveStates) {
            delete it;
        }
    }

    const set<State *> &getAllStates() const {
        return aliveStates;
    }

    /**
     * Get the count of allocated states (may or may not get released already)
     */
    int getAllocatedStateCount() const { return totalAllocatedStateCount; }

    int getAliveStateCount() const { return aliveStates.size(); }

private:

    int totalAllocatedStateCount = 0;

    set<State *> aliveStates;

};

}


#endif //KLEE_STATEALLOCATOR_HPP
