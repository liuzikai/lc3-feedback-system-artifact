//
// Created by liuzikai on 10/25/20.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "klc3/FlowAnalysis/CoverageTracker.h"

namespace klc3 {

CoverageTracker::CoverageTracker(FlowGraph *fg) : fg(fg) {
    for (const auto &edge : fg->allEdges()) {
        if (edge->type() < Edge::TYPE_RUNTIME_COUNT) {
            assert(!edge->covered);
            totalEdgeToCover++;
        }
    }
    fg->registerNewEdgeCallback(&CoverageTracker::classNewEdgeCallBack, this);
}

void CoverageTracker::updateGraphAndCoverage(State *state) {

    if (state->status != State::BROKEN) {  // executed successfully

        Edge *coveredEdge = fg->getJustCoveredEdge(state);

        if (state->status == State::HALTED) {

            if (coveredEdge == nullptr) {

                // HALTED state, getJustCoveredEdge() return nullptr if no matching virtual edge to halt
                // Issue will be raised at PostChecker

            } else {

                appendEdgeCoverage(coveredEdge, state);

                if (!state->latestInst[0].isNull() &&
                    state->latestInst[0]->sourceContent == "INSTANT_HALT") {
                    // For INSTANT_HALT, also cover the previous edge
                    if (!state->latestNonOSInst[1].isNull()) {
                        for (auto &e : state->latestNonOSInst[1]->node->runtimeOutEdges()) {
                            if (e->to() == state->latestNonOSInst[0]->node) {
                                appendEdgeCoverage(e, state);
                                break;
                            }
                        }
                    }
                }

            }

        } else {  // NORMAL

            if (coveredEdge != nullptr) {

                appendEdgeCoverage(coveredEdge, state);
                prepareEdgeToPCNode(state);

            }  // otherwise, still in virtual edge, no nothing
        }

    }  // for execution failure, nothing needs to be done
}

void CoverageTracker::prepareEdgeToPCNode(const State *state) {
    // Notice now PC node and edge to it is unprepared. Do not use getOnEdge().

    const ref<InstValue> &executedInst = state->latestNonOSInst[0];
    uint16_t pc = state->getPC();
    Node *pcNode = fg->getNodeByAddr(pc);

    if (pcNode != nullptr) {  // the node exist, no problem

        for (auto &edge : executedInst->node->runtimeOutEdges()) {
            if (edge->to() == pcNode) return;  // the edge exist, no problem
        }

        // Node exists but no edge exists
        InstValue::InstID executedInstID = executedInst->instID();
        if (executedInstID == InstValue::JMP || executedInstID == InstValue::JSRR) {
            Edge::Type edgeType;
            if (executedInstID == InstValue::JMP) {
                if (executedInst->baseR() == 7) {
                    edgeType = Edge::RET_EDGE;
                } else {
                    edgeType = Edge::JMP_EDGE;
                }
            } else {
                edgeType = Edge::JSRR_EDGE;
            }
            fg->newEdge(executedInst->node, pcNode, edgeType);
            // Let callback handle totalEdgeToCover++
            /*timedInfo() << "Dynamically identify an edge "
                        << executedInst->sourceContext() << " -> "
                        << pcNode->inst()->sourceContext() << "\n";*/
        } else {
            assert(!"Not JMP/JSRR node but no edge to PC exists");
        }

    } else {  // the PC node doesn't exist

        if (executedInst->instID() == InstValue::TRAP) {

            /*
             * Ready to go for a virtual edge, no problem, getOnEdge() will not be called.
             * Notice that you should not use state->lastNthFetchedNonOSInst[0].get() !=
             * state->lastNthFetchedInst[0].get(), since the state may just ready to step first OS node.
             */

            return;
        }

#if 1
        timedInfo() << "Detect PC pointing to non-existing node, "
                       "from: " << executedInst->sourceContext() <<
                    " to addr: " << toLC3Hex(pc) << "\n";
#endif
        // Do nothing
    }
}

void CoverageTracker::appendEdgeCoverage(Edge *edge, State *state) {
    assert(edge->type() < Edge::TYPE_RUNTIME_COUNT && "Unexpected edge type");
    if (!edge->covered) {
        coveredEdgeCount++;
        state->coveredNewEdge = true;
        edge->covered = true;
        if (edge->flags & Edge::LIKELY_UNCOVERABLE) {
            newProgWarn() << "Likely uncoverable edge " << edge->fromContext() << " -> " << edge->toContext()
                          << " is covered!\n";
        }
    }
    // The initPC edge has non-runtime type, recognized as the guiding edge
    // Include the last edge for HALTED or BROKEN
    state->statePath.appendCompressed(edge, state->status != State::NORMAL);
}

float CoverageTracker::calculateCoverage() const {
    return (float) coveredEdgeCount / (float) totalEdgeToCover;
}

void CoverageTracker::newEdgeCallback(Edge *edge) {
    if (edge->type() < Edge::TYPE_RUNTIME_COUNT) {
        totalEdgeToCover++;
    }
}

}