//
// Created by liuzikai on 4/10/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_SEARCHER_H
#define KLC3_SEARCHER_H

#include "klc3/Core/State.h"
#include "klc3/FlowAnalysis/FlowGraph.h"

#include <vector>
#include <stack>
#include <deque>

namespace klc3 {

/**
 * @note Searcher stores pointers of States, but never create or destroy them.
 */
class Searcher {
public:

    virtual ~Searcher() {}

    /**
     * Fetch a NORMAL state from the searcher. Return nullptr if the searcher is completed.
     * @return Pointer of a normal state or nullptr.
     */
    virtual State *fetch() = 0;

    /**
     * Push one or more NORMAL states to the searcher from one step of execution
     * @param states
     */
    virtual void push(const StateVector &states) = 0;

    /**
     * Get the list of all completed (HALTED or BROKEN state)
     * @return
     */
    virtual const set<State *> &getCompletedStates() const { return completedStates; }

    /**
     * Clear all completed states
     */
    virtual void clearCompletedStates() { completedStates.clear(); }

    /**
     * Push a batch of completed states back
     * @param states
     */
    virtual void eraseCompletedStates(State *s) {
        assert(s->status == State::HALTED || s->status == State::BROKEN);
        completedStates.erase(s);
    }

    /**
     * Get the list of NORMAL states
     * @return
     */
    virtual StateVector getNormalStates() const = 0;

    /**
     * Set level of fetching
     * @param level The degree of aggressiveness to explore. For example, for some searcher, some states are marked
     *               with lower priorities and can only be fetched with higher level
     */
    virtual void setLevel(int level) { (void) level; }

protected:

    std::set<State *> completedStates;

};

/**
 * Simple searcher using queue (FIFO)
 */
class SimpleFIFOSearcher : public Searcher {
public:

    State *fetch() override {
        if (normalStates.empty()) return nullptr;
        else {
            State *ret = normalStates.front();
            normalStates.pop_front();
            return ret;
        }
    }

    void push(const StateVector &states) override {
        for (auto &state : states) {
            if (state->status == State::NORMAL) {
                normalStates.push_back(state);
            } else {
                completedStates.insert(state);
            }
        }
    }

    StateVector getNormalStates() const override {
        return {normalStates.begin(), normalStates.end()};
    }

protected:

    std::deque<State *> normalStates;

};

/**
 * Simple searcher using stack (FILO)
 */
class SimpleFILOSearcher : public Searcher {
public:

    State *fetch() override {
        if (normalStates.empty()) return nullptr;
        else {
            State *ret = normalStates.back();
            normalStates.pop_back();
            return ret;
        }
    }

    void push(const StateVector &states) override {
        for (auto &state : states) {
            if (state->status == State::NORMAL) {
                normalStates.push_back(state);
            } else {
                completedStates.insert(state);
            }
        }
    }

    StateVector getNormalStates() const override {
        return {normalStates.begin(), normalStates.end()};
    }

protected:

    std::deque<State *> normalStates;
};

/**
 * FILO searcher prioritizing states that cover new edge or new segment
 */
class PrioritizedFILOSearcher : public Searcher {
public:

    State *fetch() override {
        if (!s0.empty()) {
            State *ret = s0.back();
            s0.pop_back();
            return ret;
        }
        if (!s1.empty()) {
            State *ret = s1.back();
            s1.pop_back();
            return ret;
        }
        return nullptr;
    }

    void push(const StateVector &states) override {
        for (auto &state : states) {
            if (state->status == State::NORMAL) {
                if (state->coveredNewEdge || state->coveredNewSegment) {
                    s0.push_back(state);
                } else {
                    s1.push_back(state);
                }
            } else {
                completedStates.insert(state);
            }
        }
    }

    StateVector getNormalStates() const override {
        StateVector ret(s0.begin(), s0.end());
        ret.insert(ret.begin(), s1.begin(), s1.end());
        return ret;
    }

protected:
    std::deque<State *> s0;
    std::deque<State *> s1;
};

/**
 * A searcher that randomly select one state and push it to the end.
 */
class RandomPickingSearcher : public Searcher {
public:

    RandomPickingSearcher() {
        time_t seed = std::time(nullptr);
        progInfo() << "RandomPickingSearcher: seed " << seed << "\n";
        std::srand(seed);
    }

    State *fetch() override {
        if (pickedState == nullptr && !otherNormalState.empty()) {
            // Pick a new state
            size_t id = std::rand() % otherNormalState.size();
            std::swap(otherNormalState[id], otherNormalState.back());
            pickedState = otherNormalState.back();
            otherNormalState.pop_back();
        }
        return pickedState;
    }

    void push(const StateVector &states) override {
        State *lastPickedState = pickedState;
        pickedState = nullptr;

        for (auto &state : states) {
            if (state->status == State::NORMAL) {
                if (state == lastPickedState) {  // see it again, put in pickedState
                    pickedState = state;
                } else {
                    otherNormalState.emplace_back(state);
                }
            } else {
                completedStates.insert(state);
            }
        }
    }

    StateVector getNormalStates() const override {
        StateVector ret = {otherNormalState.begin(), otherNormalState.end()};
        if (pickedState) ret.emplace_back(pickedState);
        return ret;
    }

protected:

    State *pickedState = nullptr;

    vector<State *> otherNormalState;
};

}


#endif //KLEE_SEARCHER_H
