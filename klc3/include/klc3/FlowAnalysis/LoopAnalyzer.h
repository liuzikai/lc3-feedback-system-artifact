//
// Created by liuzikai on 10/14/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef KLC3_LOOPANALYZER_H
#define KLC3_LOOPANALYZER_H

#include "klc3/FlowAnalysis/FlowGraph.h"

namespace klc3 {

// Need to be placed outside since it needs to be forward declared in FlowGraph
class Loop {
public:

    const string &name() const { return name_; }  // use entrypoint first label or pseudo label

    Node *entryNode() const { return entryNode_; }

    const auto &nodes() const { return nodes_; }

    const set<Loop *> &subloops() const { return subloops_; }

    /**
     * Note: a segment is stored as a Path (a vector of Edge) with reduced edges. The first edge, guiding edges, and
     * the last edge are stored, while the other edges are not necessary to infer the path.
     *
     * Segments are stored as Path instance since it need to be compared by value.
     *
     * h2hEdges and h2xEdges should not have duplicated edges.
     */

    const set<Path> &h2hSegments() const { return h2hSegments_; }
    const set<Edge *> &h2hEdges() const { return h2hEdges_; }

    const set<Path> &h2xSegments() const { return h2xSegments_; }
    const set<Edge *> &h2xEdges() const { return h2xEdges_; }

    const auto &segmentLastEdges() const { return segmentLastEdges_; }  // for updating loop path in PruningSearcher

    void dumpImage(FlowGraph *fg) const;

protected:
    friend class LoopAnalyzer;

    Loop() = default;  // can only be created by LoopAnalyzer

    string name_;  // use entrypoint first label or pseudo label

    Node *entryNode_ = nullptr;

    unordered_set<Node *> nodes_;

    set<Loop *> subloops_;

    set<Path> h2hSegments_;
    set<Edge *> h2hEdges_;

    set<Path> h2xSegments_;
    set<Edge *> h2xEdges_;

    unordered_set<Edge *> segmentLastEdges_;
};


class LoopAnalyzer {
public:

    explicit LoopAnalyzer(FlowGraph *fg, int maxSegmentCount = -1) : fg(fg), maxSegmentCount(maxSegmentCount) {}

    LoopAnalyzer(const LoopAnalyzer &) = delete;

    ~LoopAnalyzer();

    /**
     * Analyze loops in a subgraph. This function can be called multiple times and all loops are joint together.
     * This function should only be called when there is no statically detected improper subroutine structures.
     * @param entryNode
     * @param sg
     * @return True if analyze successfully. False if exceeds maxSegmentCount, in which case the loop results are
     *         incomplete.
     */
    bool analyzeLoops(Node *entryNode, const Subgraph &sg);

    // There is no easy way to use const Loop * since subLoops are still Loop *

    const set<Loop *> &getTopLevelLoops() const { return topLevelLoops; }

    set<Loop *> getAllLoops() const;

    int getTotalSegmentCount() const { return totalSegmentCount; }

    void dump(llvm::raw_ostream &os) const;

private:

    FlowGraph *fg;

    // ================================ Generalized Loop Detection ================================

    unordered_map<Node*, Loop *> loops;  // entry edge -> Loop, holds lifecycle

    set<Loop *> topLevelLoops;

    stack<Node *> loopHeadStack;

    int totalSegmentCount = 0;
    int maxSegmentCount = -1;

    class SCCSubgraph : public Subgraph {
    public:

        struct dfsInfo {
            int dfsPre;
            int dfsLow;
            Node *sccRoot = nullptr;
        };
        unordered_map<Node *, dfsInfo> info;

        unordered_set<Node *> loopBlockRoot;  // root of scc that contains more than one nodes

        Node *getSCCRoot(Node *node) const {
            auto it = info.find(node);
            assert(it != info.end() && "SCC root not determined");
            return it->second.sccRoot;
        }
    };

    // Return false if exceeds maxSegmentCount
    bool analyzeLoopDFS(Node *curNode, SCCSubgraph &ssg, Loop &outerLoop, Path &pathFromEntry);

    void dumpLoops(llvm::raw_ostream &os, const set<Loop *> &loops, const string &indent, int depth, int *maxDepth) const;

    static bool existingLoopCanBeSubloop(const Loop *loop, const SCCSubgraph &ssg);

    // ================================ SCC ================================

    void analysisSCC(SCCSubgraph &sccSubgraph);

    void tarjanDFS(Node *v, SCCSubgraph &sccSubgraph);

    int dfsClock = 0;

    stack<Node *> tarjanStack;
};

}

#endif //KLC3_LOOPANALYZER_H
