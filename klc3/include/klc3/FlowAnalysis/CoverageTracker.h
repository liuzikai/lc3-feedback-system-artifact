//
// Created by liuzikai on 10/25/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_COVERAGETRACKER_H
#define KLC3_COVERAGETRACKER_H

#include "FlowGraph.h"
#include "klc3/Core/State.h"

namespace klc3 {

// Forward declared dependency
class State;

class CoverageTracker {
public:

    explicit CoverageTracker(FlowGraph *fg);  // must be called after edges that need to be tracked are all constructed

    void updateGraphAndCoverage(State *state);  // may change coveredNewEdge and turningEdges

    static bool edgeNotCovered(const Edge *edge) { return !edge->covered; }

    float calculateCoverage() const;


private:

    FlowGraph *fg;

    void prepareEdgeToPCNode(const State *state);

    void appendEdgeCoverage(Edge *edge, State *state);

    int coveredEdgeCount = 0;
    int totalEdgeToCover = 0;

    void newEdgeCallback(Edge *edge);

    static void classNewEdgeCallBack(Edge *edge, void *arg) {
        static_cast<CoverageTracker *>(arg)->newEdgeCallback(edge);
    }

};

}

#endif //KLC3_COVERAGETRACKER_H
