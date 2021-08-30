//
// Created by liuzikai on 10/14/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/FlowAnalysis/FlowGraphVisualizer.h"
#include "klc3/FlowAnalysis/LoopAnalyzer.h"

namespace klc3 {

void Loop::dumpImage(FlowGraph *fg) const {
    FlowGraphVisualizer::visualizeLoop(PathString(), name(), fg, this);
}

void LoopAnalyzer::analysisSCC(SCCSubgraph &sccSubgraph) {
    dfsClock = 0;
    assert(tarjanStack.empty() && "Tarjan stack is not empty");
    sccSubgraph.loopBlockRoot.clear();
    sccSubgraph.info.clear();
    for (auto &node : sccSubgraph.nodes) {
        if (sccSubgraph.info.find(node) == sccSubgraph.info.end()) {  // not visited
            tarjanDFS(node, sccSubgraph);
        }
    }
}

void LoopAnalyzer::tarjanDFS(Node *v, SCCSubgraph &sccSubgraph) {
    ++dfsClock;

    sccSubgraph.info[v] = {
            dfsClock,
            dfsClock,
            nullptr
    };  // also marked as visited
    SCCSubgraph::dfsInfo &vInfo = sccSubgraph.info[v];

    tarjanStack.push(v);

    Node *w;
    for (auto &e : v->allOutEdges()) {
        if (e->type() == Edge::JSR_EDGE || e->type() == Edge::JSRR_EDGE) {
            continue;  // do not analyze across into subroutines, but recognize RET
        }
        // Use SUBROUTINE_VIRTUAL_EDGE in place of JSR_EDGE (there is really no JSRR_EDGE during static analysis stage).

        // Ignore loop virtual edges
        if (e->type() == Edge::LOOP_H2H_EDGE || e->type() == Edge::LOOP_H2X_EDGE) continue;

        // Ignore likely uncoverable edges
        if (e->flags & Edge::LIKELY_UNCOVERABLE) continue;

        w = e->to();
        if (w == nullptr || !sccSubgraph.containNode(w)) continue;
        if (sccSubgraph.info.find(w) == sccSubgraph.info.end()) {
            tarjanDFS(w, sccSubgraph);
            vInfo.dfsLow = std::min(vInfo.dfsLow, sccSubgraph.info[w].dfsLow);
        } else if (sccSubgraph.info[w].sccRoot == nullptr) {
            vInfo.dfsLow = std::min(vInfo.dfsLow, sccSubgraph.info[w].dfsPre);
        }
    }

    if (vInfo.dfsLow == vInfo.dfsPre) {
        int sccNodeCount = 0;
        do {
            w = tarjanStack.top();
            tarjanStack.pop();
            sccSubgraph.info[w].sccRoot = v;
            ++sccNodeCount;
        } while (w != v);
        if (sccNodeCount > 1) {
            sccSubgraph.loopBlockRoot.emplace(v);
        }
    }
}

bool LoopAnalyzer::analyzeLoops(Node *entryNode, const Subgraph &sg) {
    SCCSubgraph ssg;
    ssg.nodes = sg.nodes;
    analysisSCC(ssg);

    Loop *topLevelPseudoLoop = new Loop;  // container for top-level loops, but not push loopHead onto stack
    topLevelPseudoLoop->name_ = "(main)";
    topLevelPseudoLoop->entryNode_ = entryNode;  // take an arbitrary in Edge

    Path topLevelPseudoPath;
    bool ret = analyzeLoopDFS(entryNode, ssg, *topLevelPseudoLoop, topLevelPseudoPath);
    // If exceeds maxSegmentCount, return false

    if (ret) {
        topLevelLoops.insert(topLevelPseudoLoop->subloops().begin(), topLevelPseudoLoop->subloops().end());
    }
    delete topLevelPseudoLoop;

    return ret;
}

bool LoopAnalyzer::analyzeLoopDFS(Node *curNode, SCCSubgraph &ssg, Loop &outerLoop, Path &pathFromEntry) {

    // ssg contains all nodes in outerLoop except its entryNode (loopHeadStack.top())

    Loop *loop = nullptr;  // loop with curNode as entry (if there is one)

    auto it = loops.find(curNode);

    if (it == loops.end()) {  // not a loop, or the loop is not created yet

        if (ssg.loopBlockRoot.find(ssg.getSCCRoot(curNode)) != ssg.loopBlockRoot.end()) {
            // Is the entry point for an SCC with > 1 nodes

            // Construct new loop information structure
            loop = new Loop;
            loop->name_ = curNode->inst()->getLabel();
            loop->entryNode_ = curNode;
            Node *loopSCCNode = ssg.getSCCRoot(curNode);
            for (auto &node : ssg.nodes) {
                if (ssg.getSCCRoot(node) == loopSCCNode) {
                    loop->nodes_.emplace(node);
                }
            }
            loops.emplace(curNode, loop);
            outerLoop.subloops_.emplace(loop);

            // Construct a subgraph by eliminate the current node
            SCCSubgraph subSG;
            for (auto &node : ssg.nodes) {
                if (ssg.getSCCRoot(node) == loopSCCNode && node != curNode) {
                    subSG.nodes.emplace(node);
                }
            }
            analysisSCC(subSG);

            // Search for nested loops and segment of the newly created loop
            loopHeadStack.push(curNode);
            Path newPath;
            if (!analyzeLoopDFS(curNode, subSG, *loop, newPath)) return false;
            assert(!loop->h2hSegments().empty() && "No H2H segment found");
            // Can have no h2x segment: infinite loop, edge to data, etc.

            // The entry node may have a outEdge that immediately exit the loop
            loopHeadStack.pop();

        }  // Otherwise, leave loop as nullptr

    } else {  // encounter a loop

        loop = it->second;

        if (outerLoop.subloops().find(loop) == outerLoop.subloops().end()) {
            if (existingLoopCanBeSubloop(loop, ssg)) {
                // Not yet a subloop but can be a subloop
                outerLoop.subloops_.emplace(loop);
            } else {
                loop = nullptr;
                // Clear loop for later code to check whether encounter a subloop
            }
        }

    }

    // If loop if not nullptr, it's a subloop of outerLoop now

    vector<Edge *> originalOutEdges = curNode->allOutEdges();  // make a copy in case new segment edges are constructed
    for (auto &edge : originalOutEdges) {
        if (edge->type() == Edge::JSR_EDGE || edge->type() == Edge::JSRR_EDGE)
            continue;  // do not analyze across into subroutines, but recognize RET

        // We will go for SUBROUTINE_VIRTUAL_EDGE, assuming the subroutine returns properly .

        if (edge->flags & Edge::LIKELY_UNCOVERABLE) continue;

        if (loop) {  // encounter subloop

            // only deal with H2X segments of THE subloop
            // We still need to check the edge is for THE loop, since the node may be the entry for another loop from
            // different entry edge
            if (edge->type() != Edge::LOOP_H2X_EDGE || loop->h2xEdges().find(edge) == loop->h2xEdges().end()) {
                continue;
            }

        } else {

            // May have loop segments, since two loops can overlap but with different entry points
            // But we ignore them since we are not coming at the entry point
            if (edge->type() == Edge::LOOP_H2H_EDGE || edge->type() == Edge::LOOP_H2X_EDGE) {
                continue;
            }

        }

        if (ssg.containNode(edge->to())) {  // still inside the loop

            // Keep on searching subloops and segments for outerLoop
            bool edgeAppended = pathFromEntry.appendCompressed(edge);
            if (!analyzeLoopDFS(edge->to(), ssg, outerLoop, pathFromEntry)) return false;
            if (edgeAppended) {
                pathFromEntry.pop_back();
            }

        } else if (!loopHeadStack.empty() && edge->to() == loopHeadStack.top()) {  // detect an H2H segment

            assert(loopHeadStack.top() == outerLoop.entryNode() && "Loop head inconsistent");

            Path h2hPath = pathFromEntry;  // make a copy
            h2hPath.appendCompressed(edge, true);  // the last edge always append
            h2hPath.tag = "H-" + std::to_string(outerLoop.h2hSegments().size());

            // Create a subloop H2H segment
            assert(h2hPath.src() == outerLoop.entryNode() && h2hPath.dest() == outerLoop.entryNode() &&
                   "pathFromEntry is not a H2H path");

            bool res = outerLoop.h2hSegments_.emplace(h2hPath).second;
            assert(res && "Duplicated h2hPath");
            if (++totalSegmentCount > maxSegmentCount) return false;
            outerLoop.segmentLastEdges_.emplace(h2hPath.back());

            // If there is no a H2HEdge that is already constructed, then construct it
            Edge *h2hEdge = nullptr;
            for (auto &e : outerLoop.h2hEdges()) {
                if (e->from() == h2hPath.src() && e->to() == h2hPath.dest()) {
                    h2hEdge = e;
                    break;
                }
            }
            if (h2hEdge == nullptr) {
                h2hEdge = fg->newEdge(h2hPath.src(), h2hPath.dest(), Edge::LOOP_H2H_EDGE);
                outerLoop.h2hEdges_.emplace(h2hEdge);
            }

        } else {  // goes outside the loop, detect an H2X segment

            Path h2xPath = pathFromEntry;  // make a copy
            h2xPath.appendCompressed(edge, true);  // the last edge always append
            h2xPath.tag = "X-" + std::to_string(outerLoop.h2xSegments().size());

            // Create a subloop H2X segment
            assert(h2xPath.src() == outerLoop.entryNode() && "pathFromEntry is not from entry");

            bool res = outerLoop.h2xSegments_.emplace(h2xPath).second;
            assert(res && "Duplicated h2xPath");
            if (++totalSegmentCount > maxSegmentCount) return false;
            outerLoop.segmentLastEdges_.emplace(h2xPath.back());

            // If there is no a H2XEdge that is already constructed, then construct it
            Edge *h2xEdge = nullptr;
            for (auto &e : outerLoop.h2xEdges()) {
                if (e->from() == h2xPath.src() && e->to() == h2xPath.dest()) {
                    h2xEdge = e;
                    break;
                }
            }
            if (h2xEdge == nullptr) {
                h2xEdge = fg->newEdge(h2xPath.src(), h2xPath.dest(), Edge::LOOP_H2X_EDGE);
                outerLoop.h2xEdges_.emplace(h2xEdge);
            }
        }
    }

    return true;
}

bool LoopAnalyzer::existingLoopCanBeSubloop(const Loop *loop, const SCCSubgraph &ssg) {
    if (loop->nodes().size() > ssg.nodes.size()) return false;
    for (const auto &node : loop->nodes()) {
        if (!ssg.containNode(node)) return false;
    }
    return true;
}

set<Loop *> LoopAnalyzer::getAllLoops() const {
    set<Loop *> ret;
    for (auto it : loops) {
        ret.emplace(it.second);
    }
    return ret;
}

LoopAnalyzer::~LoopAnalyzer() {
    for (auto &it : loops) delete it.second;
}

void LoopAnalyzer::dump(llvm::raw_ostream &os) const {
    int maxDepth = 0;
    int h2hCount = 0, h2xCount = 0;

    dumpLoops(os, topLevelLoops, "", 0, &maxDepth);

    os << "Total loop count: " << loops.size() << "\n";
    os << "Max loop depth: " << maxDepth << "\n";

    for (const auto &it : loops) {
        h2hCount += it.second->h2hSegments().size();
        h2xCount += it.second->h2xSegments().size();
    }
    os << "Total H2H count: " << h2hCount << "\n";
    os << "Total H2X count: " << h2xCount << "\n";
}

void LoopAnalyzer::dumpLoops(llvm::raw_ostream &os, const set<Loop *> &ls, const string &indent, int depth, int *maxDepth) const {
    if (maxDepth && *maxDepth < depth) *maxDepth = depth;
    for (const auto &loop : ls) {
        os << indent << "-> " << loop->name() << "\n";
        os << indent << "    Size: " << loop->nodes().size() << "\n";
        os << indent << "    H2H segment count: " << loop->h2hSegments().size() << "\n";
        os << indent << "    H2H edge count: " << loop->h2hEdges().size() << "\n";
        os << indent << "    H2X segment count: " << loop->h2xSegments().size() << "\n";
        os << indent << "    H2X edge count: " << loop->h2xEdges().size() << "\n";
        dumpLoops(os, loop->subloops(), indent + "    ", depth + 1, maxDepth);
    }
}

}