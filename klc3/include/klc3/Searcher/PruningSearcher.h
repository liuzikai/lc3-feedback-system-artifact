//
// Created by liuzikai on 11/16/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_PRUNINGSEARCHER_H
#define KLC3_PRUNINGSEARCHER_H

#include "klc3/Searcher/Searcher.h"
#include "klc3/FlowAnalysis/LoopAnalyzer.h"

namespace klc3 {

class PruningSearcher : public Searcher {
public:

    explicit PruningSearcher(const FlowGraph *fg, Searcher *internalSearcher,
                             const set<Loop *> &topLevelLoops, const set<Loop *> &allLoops);

    ~PruningSearcher() override;

    State* fetch() override;

    void push(const StateVector &states) override;

    StateVector getNormalStates() const override;

    void setLevel(int level) override { fetchLevel = level; }

    const set<State *> &getCompletedStates() const override { return internalSearcher->getCompletedStates(); }

    void clearCompletedStates() override { internalSearcher->clearCompletedStates(); }

    void eraseCompletedStates(State *s) override { internalSearcher->eraseCompletedStates(s); }

private:

    const FlowGraph *fg;
    Searcher *internalSearcher;

    StateVector postponedStates;

    int fetchLevel = 1;  // by default, fetch for postponed state

    set<Loop *> topLevelLoops;

    struct LoopInfo {
        Loop *loop;

        // Need to be able to compare by value
        set<Path> uncoveredH2HSegments;
        set<Path> uncoveredH2XSegments;

        set<Edge *> subloopEdges;

        LoopInfo() : loop(nullptr) {}
        explicit LoopInfo(Loop *loop) : loop(loop) {
            for (const auto &path : loop->h2hSegments()) uncoveredH2HSegments.emplace(path);
            for (const auto &path : loop->h2xSegments()) uncoveredH2XSegments.emplace(path);
            for (const auto &subloop : loop->subloops()) {
                subloopEdges.insert(subloop->h2hEdges().begin(), subloop->h2hEdges().end());
                subloopEdges.insert(subloop->h2xEdges().begin(), subloop->h2xEdges().end());
            }
        }
    };

    unordered_map<Loop*, LoopInfo> uncoveredLoops;

    static Loop *findLoop(const set<Loop *> &loops, const Node *entryNode);

    void appendLoopPath(State *s, Edge *onEdge) const;

    void enterNewLoopIfAny(State *s, Edge *onEdge) const;

    void testLoopExitAndUpdateH2XCoverage(State *s, Edge *onEdge);

    void testH2HAndUpdate(State *s, Edge *onEdge);

    bool onPrefixOfUncoveredSegment(State *s);

    /**
     * Check whether {path, additional (optional)} is a prefix of target
     * @param path
     * @param target
     * @param additional
     * @return
     */
    static bool isPrefix(const Path &path, const Path &target, const Edge *additional = nullptr);

    State *fetchOneState();

    static bool insideTopLoop(State *s, Edge *onEdge);

    int statPostponeCount = 0;
    int statReactivateCount = 0;

    bool startedFetchingPostponedStates = false;
};

}


#endif  // KLC3_PRUNINGSEARCHER_H
